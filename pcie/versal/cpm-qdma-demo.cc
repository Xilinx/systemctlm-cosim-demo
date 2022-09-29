/*
 * Copyright (C) 2022, Advanced Micro Devices, Inc.
 * Written by Fred Konrad
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "systemc.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

#include "tlm-modules/pcie-controller.h"
#include "soc/pci/core/pcie-root-port.h"
#include "soc/pci/xilinx/qdma.h"
#include "memory.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "trace.h"
#include "iconnect.h"
#include "debugdev.h"

#include "remote-port-tlm.h"
#include "remote-port-tlm-pci-ep.h"

#define PCI_VENDOR_ID_XILINX		(0x10ee)
#define PCI_DEVICE_ID_XILINX_EF100	(0xd004)
#define PCI_SUBSYSTEM_ID_XILINX_TEST	(0x000A)

#define PCI_CLASS_BASE_NETWORK_CONTROLLER     (0x02)

#ifndef PCI_EXP_LNKCAP_ASPM_L0S
#define PCI_EXP_LNKCAP_ASPM_L0S 0x00000400 /* ASPM L0s Support */
#endif

#define KiB (1024)
#define RAM_SIZE (4 * KiB)

#define NR_MMIO_BAR  6
#define NR_IRQ       NR_QDMA_IRQ

#ifdef QDMA_CPM4_VERSION
#define QDMA_TYPE qdma_cpm4
#else
#define QDMA_TYPE qdma_cpm5
#endif

template<typename QDMA_t>
class pcie_versal : public pci_device_base
{
private:
	QDMA_t qdma;

	// BARs towards the QDMA
	tlm_utils::simple_initiator_socket<pcie_versal> user_bar_init_socket;
	tlm_utils::simple_initiator_socket<pcie_versal> cfg_init_socket;

	// QDMA towards PCIe interface (host)
	tlm_utils::simple_target_socket<pcie_versal> brdg_dma_tgt_socket;

	// MSI-X propagation
	sc_vector<sc_signal<bool> > signals_irq;

	//
	// Nothing to attach to the QDMA yet, just add a dummy memory.
	// With that the testcase will be able to check what has been
	// written in the memory..  Add an interconnect, so we can map
	// it anywhere.
	//
	iconnect<1, 1> bus;
	memory sbi_dummy;

	void bar_b_transport(int bar_nr, tlm::tlm_generic_payload &trans,
				sc_time &delay)
	{
		switch (bar_nr) {
			case QDMA_USER_BAR_ID:
				user_bar_init_socket->b_transport(trans, delay);
				break;
			case 0:
				cfg_init_socket->b_transport(trans, delay);
				break;
			default:
				SC_REPORT_ERROR("pcie_versal",
					"writing to an unimplemented bar");
				trans.set_response_status(
					tlm::TLM_GENERIC_ERROR_RESPONSE);
				break;
		}
	}

	//
	// Forward DMA requests received from the CPM5 QDMA
	//
	void fwd_dma_b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay)
	{
		dma->b_transport(trans, delay);
	}

	//
	// MSI-X propagation
	//
	void irq_thread(unsigned int i)
	{
		while (true) {
			wait(signals_irq[i].value_changed_event());
			irq[i].write(signals_irq[i].read());
		}
	}

public:
	SC_HAS_PROCESS(pcie_versal);

	pcie_versal(sc_core::sc_module_name name) :

		pci_device_base(name, NR_MMIO_BAR, NR_IRQ),

		qdma("qdma"),

		user_bar_init_socket("user_bar_init_socket"),
		cfg_init_socket("cfg_init_socket"),
		brdg_dma_tgt_socket("brdg-dma-tgt-socket"),

		signals_irq("signals_irq", NR_IRQ),

		bus("bus"),
		sbi_dummy("sbi_dummy", sc_time(0, SC_NS), RAM_SIZE)
	{
		//
		// QDMA connections
		//
		user_bar_init_socket.bind(qdma.user_bar);
		cfg_init_socket.bind(qdma.config_bar);

		// Setup DMA forwarding path (qdma.dma -> upstream to host)
		qdma.dma.bind(brdg_dma_tgt_socket);
		brdg_dma_tgt_socket.register_b_transport(
			this, &pcie_versal::fwd_dma_b_transport);

		// Connect the SBI dummy RAM
		bus.memmap(0x102100000ULL, 0x1000 - 1,
			   ADDRMODE_RELATIVE, -1, sbi_dummy.socket);
		qdma.card_bus.bind((*bus.t_sk[0]));

		// Setup MSI-X propagation
		for (unsigned int i = 0; i < NR_IRQ; i++) {
			qdma.irq[i](signals_irq[i]);
			sc_spawn(sc_bind(&pcie_versal::irq_thread, this, i));
		}
	}

	void rst(sc_signal<bool>& rst)
	{
		qdma.rst(rst);
	}
};

PhysFuncConfig getPhysFuncConfig()
{
	PhysFuncConfig cfg;
	PMCapability pmCap;
	PCIExpressCapability pcieCap;
	MSIXCapability msixCap;
	uint32_t bar_flags = PCI_BASE_ADDRESS_MEM_TYPE_64;
	uint32_t msixTableSz = NR_IRQ;
	uint32_t tableOffset = 0x100 | 4; // Table offset: 0, BIR: 4
	uint32_t pba = 0x140000 | 4; // BIR: 4
	uint32_t maxLinkWidth;

	cfg.SetPCIVendorID(PCI_VENDOR_ID_XILINX);
	// QDMA
	cfg.SetPCIDeviceID(0x903F);

	cfg.SetPCIClassProgIF(0);
	cfg.SetPCIClassDevice(0);
	cfg.SetPCIClassBase(PCI_CLASS_BASE_NETWORK_CONTROLLER);

	cfg.SetPCIBAR0(256 * KiB, bar_flags);
	cfg.SetPCIBAR2(256 * KiB, bar_flags);
	cfg.SetPCIBAR4(256 * KiB, bar_flags);

	cfg.SetPCISubsystemVendorID(PCI_VENDOR_ID_XILINX);
	cfg.SetPCISubsystemID(PCI_SUBSYSTEM_ID_XILINX_TEST);
	cfg.SetPCIExpansionROMBAR(0, 0);

	cfg.AddPCICapability(pmCap);

	maxLinkWidth = 1 << 4;
	pcieCap.SetDeviceCapabilities(PCI_EXP_DEVCAP_RBER);
	pcieCap.SetLinkCapabilities(PCI_EXP_LNKCAP_SLS_2_5GB | maxLinkWidth
				    | PCI_EXP_LNKCAP_ASPM_L0S);
	pcieCap.SetLinkStatus(PCI_EXP_LNKSTA_CLS_2_5GB | PCI_EXP_LNKSTA_NLW_X1);
	cfg.AddPCICapability(pcieCap);

	msixCap.SetMessageControl(msixTableSz-1);
	msixCap.SetTableOffsetBIR(tableOffset);
	msixCap.SetPendingBitArray(pba);
	cfg.AddPCICapability(msixCap);

	return cfg;
}

// Host / PCIe RC
//
// This pcie_host uses Remote-port to connect to a QEMU PCIe RC.
// If you'd like to connect this demo to something else, you need
// to replace this implementation with the host model you've got.
//
SC_MODULE(pcie_host)
{
private:
	remoteport_tlm_pci_ep rp_pci_ep;

public:
	pcie_root_port rootport;
	sc_in<bool> rst;

	pcie_host(sc_module_name name, const char *sk_descr) :
		sc_module(name),
		rp_pci_ep("rp-pci-ep", 0, 1, 0, sk_descr),
		rootport("rootport"),
		rst("rst")
	{
		rp_pci_ep.rst(rst);
		rp_pci_ep.bind(rootport);
	}
};

SC_MODULE(Top)
{
public:
	SC_HAS_PROCESS(Top);

	pcie_host host;

	PCIeController pcie_ctlr;
	pcie_versal<QDMA_TYPE> qdma;

	//
	// Reset signal.
	//
	sc_signal<bool> rst;

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		sc_module(name),
		host("host", sk_descr),
		pcie_ctlr("pcie-ctlr", getPhysFuncConfig()),
		qdma("pcie-qdma"),
		rst("rst")
	{
		m_qk.set_global_quantum(quantum);

		// Setup TLP sockets (host.rootport <-> pcie-ctlr)
		host.rootport.init_socket.bind(pcie_ctlr.tgt_socket);
		pcie_ctlr.init_socket.bind(host.rootport.tgt_socket);

		//
		// PCIeController <-> QDMA connections
		//
		pcie_ctlr.bind(qdma);

		// Reset signal
		host.rst(rst);
		qdma.rst(rst);

		SC_THREAD(pull_reset);
	}

	void pull_reset(void) {
		/* Pull the reset signal.  */
		rst.write(true);
		wait(1, SC_US);
		rst.write(false);
	}

private:
	tlm_utils::tlm_quantumkeeper m_qk;
};

void usage(void)
{
	cout << "tlm socket-path sync-quantum-ns" << endl;
}

int sc_main(int argc, char* argv[])
{
	Top *top;
	uint64_t sync_quantum;
	sc_trace_file *trace_fp = NULL;

	if (argc < 3) {
		sync_quantum = 10000;
	} else {
		sync_quantum = strtoull(argv[2], NULL, 10);
	}

	sc_set_time_resolution(1, SC_PS);

	top = new Top("top", argv[1], sc_time((double) sync_quantum, SC_NS));

	if (argc < 3) {
		sc_start(1, SC_PS);
		sc_stop();
		usage();
		exit(EXIT_FAILURE);
	}

	trace_fp = sc_create_vcd_trace_file("trace");
	if (trace_fp) {
		trace(trace_fp, *top, top->name());
	}

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}
	return 0;
}
