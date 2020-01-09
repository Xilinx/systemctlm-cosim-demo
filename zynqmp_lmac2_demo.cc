/*
 * Top level of the ZynqMP LMAC_CORE2 cosim example.
 *
 * Copyright (c) 2019 Xilinx Inc.
 * Written by Edgar E. Iglesias and Francisco Iglesias.
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

#include <inttypes.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "systemc.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"

using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "trace.h"
#include "iconnect.h"
#include "memory.h"
#include "xilinx-axidma.h"
#include "xilinx-zynqmp.h"

#include "tlm-bridges/tlm2apb-bridge.h"
#include "tlm-bridges/tlm2axis-bridge.h"
#include "tlm-bridges/axis2tlm-bridge.h"
#include "tlm-xgmii-phy.h"

#ifdef HAVE_VERILOG_VERILATOR
#include <verilated_vcd_sc.h>
#include "Vlmac_wrapper_top.h"
#include "verilated.h"
#endif

#define NR_MASTERS	3
#define NR_DEVICES	4

#ifndef BASE_ADDR
#define BASE_ADDR	0xa0000000ULL
#endif

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	iconnect<NR_MASTERS, NR_DEVICES>	*bus;
	xilinx_zynqmp zynq;

	axidma_mm2s dma_mm2s_A;
	axidma_s2mm dma_s2mm_C;

	tlm2axis_bridge<64> tlm2axis;
	axis2tlm_bridge<64> axis2tlm;

	sc_signal<bool> rst, rst_n;

	sc_clock *clk;

	tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32> *tlm2apb_lmac;

	Vlmac_wrapper_top *lmac;
	tlm_xgmii_phy phy;

	sc_signal<sc_bv<64> > phy_txd;
	sc_signal<sc_bv<8> > phy_txc;
	sc_signal<sc_bv<64> > phy_rxd;
	sc_signal<sc_bv<8> > phy_rxc;

	sc_signal<sc_bv<64> > tx_axis_mac_tdata;
	sc_signal<sc_bv<8> > tx_axis_mac_tstrb;
	sc_signal<bool> tx_axis_mac_tvalid;
	sc_signal<bool> tx_axis_mac_tlast;
	sc_signal<bool> tx_axis_mac_tuser;
	sc_signal<bool> tx_axis_mac_tready;

	sc_signal<sc_bv<64> > rx_axis_mac_tdata;
	sc_signal<sc_bv<8> > rx_axis_mac_tstrb;
	sc_signal<bool> rx_axis_mac_tvalid;
	sc_signal<bool> rx_axis_mac_tlast;
	sc_signal<bool> rx_axis_mac_tuser;
	sc_signal<bool> rx_axis_mac_tready;

	sc_signal<bool> apbsig_lmac_psel;
	sc_signal<bool> apbsig_lmac_penable;
	sc_signal<bool> apbsig_lmac_pwrite;
	sc_signal<bool> apbsig_lmac_pready;
	sc_signal<sc_bv<16> > apbsig_lmac_paddr;
	sc_signal<sc_bv<32> > apbsig_lmac_pwdata;
	sc_signal<sc_bv<32> > apbsig_lmac_prdata;

	sc_signal<bool> apbsig_timer_psel;
	sc_signal<bool> apbsig_timer_penable;
	sc_signal<bool> apbsig_timer_pwrite;
	sc_signal<bool> apbsig_timer_pready;
	sc_signal<sc_bv<16> > apbsig_timer_paddr;
	sc_signal<sc_bv<32> > apbsig_timer_pwdata;
	sc_signal<sc_bv<32> > apbsig_timer_prdata;

	void gen_rst_n(void)
	{
		rst_n.write(!rst.read());
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		zynq("zynq", sk_descr, remoteport_tlm_sync_untimed_ptr),
		dma_mm2s_A("dma_mm2s_A"),
		dma_s2mm_C("dma_s2mm_C"),
		tlm2axis("tlm2axis"),
		axis2tlm("axis2tlm"),
		rst("rst"),
		rst_n("rst_n"),

		phy("phy"),
		phy_txd("phy_txd"),
		phy_txc("phy_txc"),
		phy_rxd("phy_rxd"),
		phy_rxc("phy_rxc"),

		tx_axis_mac_tdata("tx_axis_mac_tdata"),
		tx_axis_mac_tstrb("tx_axis_mac_tstrb"),
		tx_axis_mac_tvalid("tx_axis_mac_tvalid"),
		tx_axis_mac_tlast("tx_axis_mac_tlast"),
		tx_axis_mac_tuser("tx_axis_mac_tuser"),
		tx_axis_mac_tready("tx_axis_mac_tready"),

		rx_axis_mac_tdata("rx_axis_mac_tdata"),
		rx_axis_mac_tstrb("rx_axis_mac_tstrb"),
		rx_axis_mac_tvalid("rx_axis_mac_tvalid"),
		rx_axis_mac_tlast("rx_axis_mac_tlast"),
		rx_axis_mac_tuser("rx_axis_mac_tuser"),
		rx_axis_mac_tready("rx_axis_mac_tready"),

		apbsig_lmac_psel("apblmac_psel"),
		apbsig_lmac_penable("apblmac_penable"),
		apbsig_lmac_pwrite("apblmac_pwrite"),
		apbsig_lmac_pready("apblmac_pready"),
		apbsig_lmac_paddr("apblmac_paddr"),
		apbsig_lmac_pwdata("apblmac_pwdata"),
		apbsig_lmac_prdata("apblmac_prdata"),

		apbsig_timer_psel("apbtimer_psel"),
		apbsig_timer_penable("apbtimer_penable"),
		apbsig_timer_pwrite("apbtimer_pwrite"),
		apbsig_timer_pready("apbtimer_pready"),
		apbsig_timer_paddr("apbtimer_paddr"),
		apbsig_timer_pwdata("apbtimer_pwdata"),
		apbsig_timer_prdata("apbtimer_prdata")
	{
		unsigned int i;

		SC_METHOD(gen_rst_n);
		sensitive << rst;

		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);

		bus   = new iconnect<NR_MASTERS, NR_DEVICES> ("bus");

		tlm2apb_lmac = new tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32> ("tlm2apb-lmac-bridge");
		bus->memmap(BASE_ADDR + 0x30000ULL, 0x4000 - 1,
				ADDRMODE_RELATIVE, -1, tlm2apb_lmac->tgt_socket);

		bus->memmap(BASE_ADDR + 0x34000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, dma_mm2s_A.tgt_socket);
		bus->memmap(BASE_ADDR + 0x35000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, dma_s2mm_C.tgt_socket);

		bus->memmap(0x0LL, 0xffffffff - 1,
				ADDRMODE_RELATIVE, -1, *(zynq.s_axi_hpc_fpd[0]));

		zynq.s_axi_hpm_fpd[0]->bind(*(bus->t_sk[0]));

		dma_mm2s_A.init_socket.bind(*(bus->t_sk[1]));
		dma_s2mm_C.init_socket.bind(*(bus->t_sk[2]));

		dma_mm2s_A.stream_socket.bind(tlm2axis.tgt_socket);
		axis2tlm.socket.bind(dma_s2mm_C.stream_socket);

		dma_mm2s_A.irq(zynq.pl2ps_irq[2]);
		dma_s2mm_C.irq(zynq.pl2ps_irq[4]);

		/* Slow clock to keep simulation fast.  */
		clk = new sc_clock("clk", sc_time(10, SC_US));
		lmac = new Vlmac_wrapper_top("lmac");

		tlm2axis.clk(*clk);
		tlm2axis.resetn(rst_n);
		tlm2axis.tvalid(tx_axis_mac_tvalid);
		tlm2axis.tready(tx_axis_mac_tready);
		tlm2axis.tdata(tx_axis_mac_tdata);
		tlm2axis.tstrb(tx_axis_mac_tstrb);
		tlm2axis.tuser(tx_axis_mac_tuser);
		tlm2axis.tlast(tx_axis_mac_tlast);

		axis2tlm.clk(*clk);
		axis2tlm.resetn(rst_n);
		axis2tlm.tvalid(rx_axis_mac_tvalid);
		axis2tlm.tready(rx_axis_mac_tready);
		axis2tlm.tdata(rx_axis_mac_tdata);
		axis2tlm.tstrb(rx_axis_mac_tstrb);
		axis2tlm.tuser(rx_axis_mac_tuser);
		axis2tlm.tlast(rx_axis_mac_tlast);

		lmac->clk(*clk);
		lmac->rst_n(rst_n);
		lmac->xgmii_txd(phy_txd);
		lmac->xgmii_txc(phy_txc);
		lmac->xgmii_rxd(phy_rxd);
		lmac->xgmii_rxc(phy_rxc);
		lmac->host_addr_reg(apbsig_lmac_paddr);
		lmac->reg_rd_start(apbsig_lmac_psel);
		lmac->reg_rd_done_out(apbsig_lmac_pready);
		lmac->FMAC_REGDOUT(apbsig_lmac_prdata);

		lmac->tx_axis_mac_tdata(tx_axis_mac_tdata);
		lmac->tx_axis_mac_tvalid(tx_axis_mac_tvalid);
		lmac->tx_axis_mac_tlast(tx_axis_mac_tlast);
		lmac->tx_axis_mac_tuser(tx_axis_mac_tuser);
		lmac->tx_axis_mac_tready(tx_axis_mac_tready);
		lmac->tx_axis_mac_tstrb(tx_axis_mac_tstrb);

		lmac->rx_axis_mac_tdata(rx_axis_mac_tdata);
		lmac->rx_axis_mac_tvalid(rx_axis_mac_tvalid);
		lmac->rx_axis_mac_tlast(rx_axis_mac_tlast);
		lmac->rx_axis_mac_tuser(rx_axis_mac_tuser);
		lmac->rx_axis_mac_tready(rx_axis_mac_tready);
		lmac->rx_axis_mac_tstrb(rx_axis_mac_tstrb);

		phy.tx.clk(*clk);
		phy.tx.xxd(phy_txd);
		phy.tx.xxc(phy_txc);

		phy.rx.clk(*clk);
		phy.rx.xxd(phy_rxd);
		phy.rx.xxc(phy_rxc);

		zynq.user_master[0]->bind(phy.rx.tgt_socket);
		phy.tx.init_socket.bind(*zynq.user_slave[0]);

		tlm2apb_lmac->clk(*clk);
		tlm2apb_lmac->psel(apbsig_lmac_psel);
		tlm2apb_lmac->penable(apbsig_lmac_penable);
		tlm2apb_lmac->pwrite(apbsig_lmac_pwrite);
		tlm2apb_lmac->paddr(apbsig_lmac_paddr);
		tlm2apb_lmac->pwdata(apbsig_lmac_pwdata);
		tlm2apb_lmac->prdata(apbsig_lmac_prdata);
		tlm2apb_lmac->pready(apbsig_lmac_pready);

		zynq.tie_off();
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

#if HAVE_VERILOG_VERILATOR
	Verilated::commandArgs(argc, argv);
#endif

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
	trace(trace_fp, *top, top->name());

#if VM_TRACE
	Verilated::traceEverOn(true);
	// If verilator was invoked with --trace argument,
	// and if at run time passed the +trace argument, turn on tracing
	VerilatedVcdSc* tfp = NULL;
	const char* flag = Verilated::commandArgsPlusMatch("trace");
	if (flag && 0 == strcmp(flag, "+trace")) {
		tfp = new VerilatedVcdSc;
		top->lmac->trace(tfp, 99);
		tfp->open("vlt_dump.vcd");
	}
#endif

	/* Pull the reset signal.  */
	top->rst.write(true);
	sc_start(1, SC_US);
	top->rst.write(false);

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}

#if VM_TRACE
	if (tfp) { tfp->close(); tfp = NULL; }
#endif
	return 0;
}
