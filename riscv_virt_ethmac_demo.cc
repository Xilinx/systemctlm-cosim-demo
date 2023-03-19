/*
 * Top level of the RISCV + OpenCores ETHMAC cosim example.
 *
 * Copyright (c) 2023 Zero ASIC.
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
#include "tlm-bridges/tlm2wishbone-bridge.h"
#include "tlm-bridges/wishbone2tlm-bridge.h"
#include "tlm-bridges/mii2tlm-bridge.h"
#include "tlm-bridges/tlm2mii-bridge.h"
#include "soc/interconnect/iconnect.h"
#include "tests/test-modules/memory.h"
#include "tests/test-modules/signals-wishbone.h"
#include "soc/xilinx/zynqmp/xilinx-zynqmp.h"

#include "Vethmac.h"
#include "verilated.h"
#include <verilated_vcd_sc.h>

#define NR_MASTERS	2
#define NR_DEVICES	2

#ifndef BASE_ADDR
#define BASE_ADDR       0x28030000LL
#endif

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	iconnect<NR_MASTERS, NR_DEVICES> bus;
	xilinx_zynqmp zynq;

	sc_signal<sc_bv<10> > sig_s_adr10;
	WishBoneSignals<32, 32> sig_s;
	WishBoneSignals<32, 32> sig_m;

	tlm2wishbone_bridge<32, 32> tlm2wb;
	wishbone2tlm_bridge<32, 32> wb2tlm;

	mii2tlm_bridge mii2tlm;
	tlm2mii_bridge tlm2mii;

	Vethmac mac;

	sc_clock clk_1G;
	sc_clock clk_100M;

	sc_signal<bool> rst, rst_n;

	sc_signal<sc_bv<4> > mrxd;
	sc_signal<bool > mrxdv;
	sc_signal<bool > mrxerr;

	sc_signal<sc_bv<4> > mtxd;
	sc_signal<bool > mtxen;
	sc_signal<bool > mtxerr;

	sc_signal<bool > mcoll;
	sc_signal<bool > mcrs;

	/* MDIO.  */
	sc_signal<bool > md_i;
	sc_signal<bool > mdc;
	sc_signal<bool > md_o;
	sc_signal<bool > md_oe_o;

	sc_signal<bool > irq;

	void pull_reset(void) {
		// No collisions
		mcoll.write(0);
		// Carrier is up
		mcrs.write(1);


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

	void gen_adr_slice(void)
	{
		sig_s_adr10.write(sig_s.adr.read() >> 2);
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		bus("bus"),
		zynq("zynq", sk_descr, remoteport_tlm_sync_untimed_ptr, false),
		sig_s_adr10("sig_s_adr10"),
		sig_s("sig_s"),
		sig_m("sig_m"),
		tlm2wb("tlm2wb", tlm2wishbone_bridge<32,32>::ENDIAN_NATIVE),
		wb2tlm("wb2tlm", wishbone2tlm_bridge<32,32>::ENDIAN_BIG),
		mii2tlm("mii2tlm"),
		tlm2mii("tlm2mii"),
		mac("mac"),
		// Scaled to allow QEMU tun run faster
		clk_1G("clk_1G", sc_time(1, SC_US)),
		clk_100M("clk_100M", sc_time(5, SC_US)),
		rst("rst"),
		rst_n("rst_n")
	{
		SC_THREAD(pull_reset);

		SC_METHOD(gen_rst_n);
		sensitive << rst;

		SC_METHOD(gen_adr_slice);
		sensitive << sig_s.adr;

		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);

		// Wire-up the ethmac
		mac.wb_clk_i(clk_1G);
		mac.wb_rst_i(rst);

		mac.wb_dat_i(sig_s.dat_o);
		mac.wb_dat_o(sig_s.dat_i);
		mac.wb_err_o(sig_s.err);

		mac.wb_adr_i(sig_s_adr10);
		mac.wb_sel_i(sig_s.sel);
		mac.wb_we_i(sig_s.we);
		mac.wb_cyc_i(sig_s.cyc);
		mac.wb_stb_i(sig_s.stb);
		mac.wb_ack_o(sig_s.ack);

		mac.m_wb_adr_o(sig_m.adr);
		mac.m_wb_sel_o(sig_m.sel);
		mac.m_wb_we_o(sig_m.we);
		mac.m_wb_dat_i(sig_m.dat_i);
		mac.m_wb_dat_o(sig_m.dat_o);
		mac.m_wb_cyc_o(sig_m.cyc);
		mac.m_wb_stb_o(sig_m.stb);
		mac.m_wb_ack_i(sig_m.ack);
		mac.m_wb_err_i(sig_m.err);

		mac.m_wb_cti_o(sig_m.cti);
		mac.m_wb_bte_o(sig_m.bte);

		mac.mtx_clk_pad_i(clk_100M);
		mac.mtxd_pad_o(mtxd);
		mac.mtxen_pad_o(mtxen);
		mac.mtxerr_pad_o(mtxerr);

		mac.mrx_clk_pad_i(clk_100M);
		mac.mrxd_pad_i(mrxd);
		mac.mrxdv_pad_i(mrxdv);
		mac.mrxerr_pad_i(mrxerr);

		mac.mcoll_pad_i(mcoll);
		mac.mcrs_pad_i(mcrs);

		mac.md_pad_i(md_i);
		mac.mdc_pad_o(mdc);
		mac.md_pad_o(md_o);
		mac.md_padoe_o(md_oe_o);

		mac.int_o(zynq.pl2ps_irq[2]);

		tlm2wb.clk_i(clk_1G);
		tlm2wb.rst_i(rst);
		sig_s.connect_master(&tlm2wb);

		wb2tlm.clk_i(clk_1G);
		wb2tlm.rst_i(rst);
		sig_m.connect_slave(&wb2tlm);
		wb2tlm.err_o(sig_m.err);
		wb2tlm.cti_i(sig_m.cti);
		wb2tlm.bte_i(sig_m.bte);

		mii2tlm.clk(clk_100M);
		mii2tlm.data(mtxd);
		mii2tlm.enable(mtxen);
		mii2tlm.init_socket.bind(*zynq.user_slave[0]);

		tlm2mii.clk(clk_100M);
		tlm2mii.data(mrxd);
		tlm2mii.enable(mrxdv);
		zynq.user_master[0]->bind(tlm2mii.socket);

		bus.memmap(BASE_ADDR, BASE_ADDR + 0x1000 - 1,
				ADDRMODE_RELATIVE, -1, tlm2wb.socket);

		bus.memmap(0x0LL, 0xffffffff - 1,
				ADDRMODE_RELATIVE, -1, *(zynq.s_axi_hpc_fpd[0]));

		zynq.s_axi_hpm_fpd[0]->bind(*(bus.t_sk[0]));
		wb2tlm.socket.bind(*(bus.t_sk[1]));

		zynq.tie_off();
	}

private:
	tlm_utils::tlm_quantumkeeper m_qk;
};

void usage(void)
{
	cout << "tlm socket-path sync-quantum-ns" << endl;
}

void signal_callback_handler(int signum) {
	cout << "Caught SIGINT" << endl;
	sc_stop();
}

int sc_main(int argc, char* argv[])
{
	Top *top;
	uint64_t sync_quantum;

#if HAVE_VERILOG_VERILATOR
        Verilated::commandArgs(argc, argv);
#endif

	if (argc < 3) {
		sync_quantum = 10000;
	} else {
		sync_quantum = strtoull(argv[2], NULL, 10);
	}

        signal(SIGINT, signal_callback_handler);

	sc_set_time_resolution(1, SC_NS);

	top = new Top("top", argv[1], sc_time((double) sync_quantum, SC_NS));

        sc_trace_file *trace_fp = sc_create_vcd_trace_file(argv[0]);

        trace(trace_fp, *top, "top");
        top->sig_s.trace(trace_fp);
        top->sig_m.trace(trace_fp);

	sc_start(SC_ZERO_TIME);

#if VM_TRACE
        Verilated::traceEverOn(true);
        // If verilator was invoked with --trace argument,
        // and if at run time passed the +trace argument, turn on tracing
        VerilatedVcdSc* tfp = NULL;
        const char* flag = Verilated::commandArgsPlusMatch("trace");
        if (flag && 0 == strcmp(flag, "+trace")) {
                tfp = new VerilatedVcdSc;
                top->mac.trace(tfp, 99);
                tfp->open("vlt_dump.vcd");
        }
#endif

	if (argc < 3) {
		sc_start(1, SC_PS);
		sc_stop();
		usage();
		delete top;
		exit(EXIT_FAILURE);
	}

#if VM_TRACE
	if (tfp) { tfp->close(); tfp = NULL; }
#endif

	sc_start();
	return 0;
}
