/*
 * Top level of the Versal MRMAC cosim example. This is based on the VCK190
 * MRMAC TRD:
 * https://github.com/Xilinx/vck190-ethernet-trd
 *
 * Copyright (c) 2022 Advanced Micro Devices.
 * Written by Edgar E. Iglesias.
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
#include "soc/interconnect/iconnect.h"
#include "tests/test-modules/memory.h"
#include "soc/xilinx/versal/xilinx-versal.h"
#include "soc/net/ethernet/xilinx/mrmac/mrmac.h"
#include "soc/dma/xilinx/mcdma/mcdma.h"

#define NR_MASTERS	3
#define NR_DEVICES	8

//#define BASE_ADDR       0x28000000ULL
#ifndef BASE_ADDR
#define BASE_ADDR	0xa4000000ULL
#endif

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	iconnect<NR_MASTERS, NR_DEVICES> bus;
	xilinx_versal versal;

	xilinx_mrmac mac;
	xilinx_mcdma dma;

	// Dummy models.
	memory gt_ctrl0;
	memory gt_ctrl1;
	memory gt_ctrl2;
	memory gt_ctrl3;
	memory gt_pll;

	sc_clock clk;
	sc_signal<bool> rst, rst_n;

	void pull_reset(void) {
		/* Pull the reset signal.  */
		rst.write(true);
		wait(100, SC_US);
		rst.write(false);
		wait(100, SC_US);
	}

	void gen_rst_n(void)
	{
		rst_n.write(!rst.read());
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		bus("bus"),
		versal("versal", sk_descr, remoteport_tlm_sync_untimed_ptr, true),
		mac("mac", true),
		dma("dma", 1),
		gt_ctrl0("gt_ctrl0", SC_ZERO_TIME, 0x100),
		gt_ctrl1("gt_ctrl1", SC_ZERO_TIME, 0x100),
		gt_ctrl2("gt_ctrl2", SC_ZERO_TIME, 0x100),
		gt_ctrl3("gt_ctrl3", SC_ZERO_TIME, 0x100),
		gt_pll("gt_pll", SC_ZERO_TIME, 0x100),
		clk("clk", sc_time(20, SC_US)),
		rst("rst"),
		rst_n("rst_n")
	{
		SC_THREAD(pull_reset);

		SC_METHOD(gen_rst_n);
		sensitive << rst;

		m_qk.set_global_quantum(quantum);

		versal.rst(rst);
		mac.rst(rst);
		dma.rst(rst);

		bus.memmap(BASE_ADDR + 0x10000ULL, 0x10000 - 1,
				ADDRMODE_RELATIVE, -1, mac.reg_socket);

		bus.memmap(BASE_ADDR + 0x20000ULL, 0x1000 - 1,
				ADDRMODE_RELATIVE, -1, dma.target_socket);

		bus.memmap(BASE_ADDR + 0x60000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, gt_ctrl0.socket);

		bus.memmap(BASE_ADDR + 0x70000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, gt_ctrl1.socket);

		bus.memmap(BASE_ADDR + 0x80000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, gt_ctrl2.socket);

		bus.memmap(BASE_ADDR + 0x90000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, gt_ctrl3.socket);

		bus.memmap(BASE_ADDR + 0xa0000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, gt_pll.socket);

		bus.memmap(0x0LL, 0xffffffff - 1,
				ADDRMODE_RELATIVE, -1, *(versal.s_axi_fpd));

		versal.m_axi_lpd->bind(*(bus.t_sk[0]));
		versal.m_axi_fpd->bind(*(bus.t_sk[1]));

		dma.init_socket.bind(*(bus.t_sk[2]));

		dma.mm2s_stream_socket[0].bind(mac.mac_tx_socket);
		mac.mac_rx_socket.bind(dma.s2mm_stream_socket[0]);

		dma.mm2s_irq(versal.pl2ps_irq[0]);
		dma.s2mm_irq(versal.pl2ps_irq[1]);

		versal.user_master[0]->bind(mac.phy_rx_socket);
		mac.phy_tx_socket.bind(*versal.user_slave[0]);

		versal.tie_off();
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
		delete top;
		exit(EXIT_FAILURE);
	}

	sc_start();
	return 0;
}
