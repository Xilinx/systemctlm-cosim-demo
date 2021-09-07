/*
 * Top level of the ZynqMP cosim example.
 *
 * Copyright (c) 2014 Xilinx Inc.
 * Written by Edgar E. Iglesias
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
#include "tests/test-modules/memory.h"
#include "debugdev.h"
#include "demo-dma.h"
#include "soc/xilinx/zynqmp/xilinx-zynqmp.h"

#include "checkers/pc-axilite.h"
#include "tlm-bridges/tlm2axilite-bridge.h"
#include "tlm-bridges/tlm2axi-bridge.h"
#include "tlm-bridges/tlm2apb-bridge.h"
#ifdef HAVE_VERILOG_VCS
#include "apb_slave_timer.h"
#endif

#ifdef HAVE_VERILOG_VERILATOR
#include "Vapb_timer.h"
#include "Vaxilite_dev.h"
#include "Vaxifull_dev.h"
#include "verilated.h"
#include <verilated_vcd_sc.h>
#endif

#define NR_DEMODMA      4
#define NR_MASTERS	1 + NR_DEMODMA
#define NR_DEVICES	6 + NR_DEMODMA

SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	iconnect<NR_MASTERS, NR_DEVICES> bus;
	xilinx_zynqmp zynq;
	memory mem;
	debugdev debug;
	demodma *dma[NR_DEMODMA];

	sc_signal<bool> rst, rst_n;

	sc_clock *clk;
#define AXIFULL_DATA_WIDTH 128
#define AXIFULL_ID_WIDTH 8
#ifdef HAVE_VERILOG
	tlm2axilite_bridge<4, 32> *tlm2axi_al;
	tlm2axi_bridge<10, AXIFULL_DATA_WIDTH> *tlm2axi_af;
	AXILiteProtocolChecker<4, 32> checker;
#else
	memory mem_al;
	memory mem_af;
#endif
	tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32> *tlm2apb_tmr;
#ifdef HAVE_VERILOG
#ifdef HAVE_VERILOG_VCS
	apb_slave_timer *apb_timer;
#endif
#ifdef HAVE_VERILOG_VERILATOR
	Vapb_timer *apb_timer;
	Vaxilite_dev *al;
	Vaxifull_dev *af;
#endif
	sc_signal<bool> irq_tmr;
#endif
	sc_signal<bool> apbsig_timer_psel;
	sc_signal<bool> apbsig_timer_penable;
	sc_signal<bool> apbsig_timer_pwrite;
	sc_signal<bool> apbsig_timer_pready;
	sc_signal<sc_bv<16> > apbsig_timer_paddr;
	sc_signal<sc_bv<32> > apbsig_timer_pwdata;
	sc_signal<sc_bv<32> > apbsig_timer_prdata;

	sc_signal<bool> al_awvalid;
	sc_signal<bool> al_awready;
	sc_signal<sc_bv<4> > al_awaddr;
	sc_signal<sc_bv<3> > al_awprot;

	sc_signal<bool> al_wvalid;
	sc_signal<bool> al_wready;
	sc_signal<sc_bv<32> > al_wdata;
	sc_signal<sc_bv<4> > al_wstrb;

	sc_signal<bool> al_bvalid;
	sc_signal<bool> al_bready;
	sc_signal<sc_bv<2> > al_bresp;

	sc_signal<bool> al_arvalid;
	sc_signal<bool> al_arready;
	sc_signal<sc_bv<4> > al_araddr;
	sc_signal<sc_bv<3> > al_arprot;

	sc_signal<bool> al_rvalid;
	sc_signal<bool> al_rready;
	sc_signal<sc_bv<32> > al_rdata;
	sc_signal<sc_bv<2> > al_rresp;

	sc_signal<bool> af_awvalid;
	sc_signal<bool> af_awready;
	sc_signal<sc_bv<10> > af_awaddr;
	sc_signal<sc_bv<3> > af_awprot;
	sc_signal<sc_bv<2> > af_awuser;
	sc_signal<sc_bv<4> > af_awregion;
	sc_signal<sc_bv<4> > af_awqos;
	sc_signal<sc_bv<4> > af_awcache;
	sc_signal<sc_bv<2> > af_awburst;
	sc_signal<sc_bv<3> > af_awsize;
	sc_signal<sc_bv<8> > af_awlen;
	sc_signal<sc_bv<AXIFULL_ID_WIDTH> > af_awid;
	sc_signal<bool > af_awlock;

	sc_signal<bool> af_wvalid;
	sc_signal<bool> af_wready;
	sc_signal<sc_bv<AXIFULL_DATA_WIDTH> > af_wdata;
	sc_signal<sc_bv<AXIFULL_DATA_WIDTH/8> > af_wstrb;
	sc_signal<sc_bv<2> > af_wuser;
	sc_signal<bool> af_wlast;

	sc_signal<bool> af_bvalid;
	sc_signal<bool> af_bready;
	sc_signal<sc_bv<2> > af_bresp;
	sc_signal<sc_bv<2> > af_buser;
	sc_signal<sc_bv<AXIFULL_ID_WIDTH> > af_bid;

	sc_signal<bool> af_arvalid;
	sc_signal<bool> af_arready;
	sc_signal<sc_bv<10> > af_araddr;
	sc_signal<sc_bv<3> > af_arprot;
	sc_signal<sc_bv<2> > af_aruser;
	sc_signal<sc_bv<4> > af_arregion;
	sc_signal<sc_bv<4> > af_arqos;
	sc_signal<sc_bv<4> > af_arcache;
	sc_signal<sc_bv<2> > af_arburst;
	sc_signal<sc_bv<3> > af_arsize;
	sc_signal<sc_bv<8> > af_arlen;
	sc_signal<sc_bv<AXIFULL_ID_WIDTH> > af_arid;
	sc_signal<bool > af_arlock;

	sc_signal<bool> af_rvalid;
	sc_signal<bool> af_rready;
	sc_signal<sc_bv<AXIFULL_DATA_WIDTH> > af_rdata;
	sc_signal<sc_bv<2> > af_rresp;
	sc_signal<sc_bv<2> > af_ruser;
	sc_signal<sc_bv<AXIFULL_ID_WIDTH> > af_rid;
	sc_signal<bool> af_rlast;

	void pull_reset(void) {
		/* Pull the reset signal.  */
		rst.write(true);
		wait(1, SC_US);
		rst.write(false);
	}

	void gen_rst_n(void)
	{
		rst_n.write(!rst.read());
	}

	AXILitePCConfig checker_config()
	{
		AXILitePCConfig cfg;

		cfg.enable_all_checks();

		return cfg;
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		bus("bus"),
		zynq("zynq", sk_descr),
		mem("mem", sc_time(1, SC_NS), 64 * 1024),
		debug("debug"),
		rst("rst"),
		rst_n("rst_n"),
#ifdef HAVE_VERILOG
		checker("checker", checker_config()),
		irq_tmr("irq_tmr"),
#else
		mem_al("mem_al", sc_time(1, SC_NS), 4 * 4),
		// 128bits x 512 slots.
		mem_af("mem_af", sc_time(1, SC_NS), 16 * 512),
#endif
		apbsig_timer_psel("apbtimer_psel"),
		apbsig_timer_penable("apbtimer_penable"),
		apbsig_timer_pwrite("apbtimer_pwrite"),
		apbsig_timer_pready("apbtimer_pready"),
		apbsig_timer_paddr("apbtimer_paddr"),
		apbsig_timer_pwdata("apbtimer_pwdata"),
		apbsig_timer_prdata("apbtimer_prdata"),

		al_awvalid("al_awvalid"),
		al_awready("al_awready"),
		al_awaddr("al_awaddr"),
		al_awprot("al_awprot"),

		al_wvalid("al_wvalid"),
		al_wready("al_wready"),
		al_wdata("al_wdata"),
		al_wstrb("al_wstrb"),

		al_bvalid("al_bvalid"),
		al_bready("al_bready"),
		al_bresp("al_bresp"),

		al_arvalid("al_arvalid"),
		al_arready("al_arready"),
		al_araddr("al_araddr"),
		al_arprot("al_arprot"),

		al_rvalid("al_rvalid"),
		al_rready("al_rready"),
		al_rdata("al_rdata"),
		al_rresp("al_rresp"),


		af_awvalid("af_awvalid"),
		af_awready("af_awready"),
		af_awaddr("af_awaddr"),
		af_awprot("af_awprot"),
		af_awuser("af_awuser"),
		af_awregion("af_awregion"),
		af_awqos("af_awqos"),
		af_awcache("af_awcache"),
		af_awburst("af_awburst"),
		af_awsize("af_awsize"),
		af_awlen("af_awlen"),
		af_awid("af_awid"),
		af_awlock("af_awlock"),

		af_wvalid("af_wvalid"),
		af_wready("af_wready"),
		af_wdata("af_wdata"),
		af_wstrb("af_wstrb"),
		af_wuser("af_wuser"),
		af_wlast("af_wlast"),

		af_bvalid("af_bvalid"),
		af_bready("af_bready"),
		af_bresp("af_bresp"),
		af_buser("af_buser"),
		af_bid("af_bid"),

		af_arvalid("af_arvalid"),
		af_arready("af_arready"),
		af_araddr("af_araddr"),
		af_arprot("af_arprot"),
		af_aruser("af_aruser"),
		af_arregion("af_arregion"),
		af_arqos("af_arqos"),
		af_arcache("af_arcache"),
		af_arburst("af_arburst"),
		af_arsize("af_arsize"),
		af_arlen("af_arlen"),
		af_arid("af_arid"),
		af_arlock("af_arlock"),

		af_rvalid("af_rvalid"),
		af_rready("af_rready"),
		af_rdata("af_rdata"),
		af_rresp("af_rresp"),
		af_ruser("af_ruser"),
		af_rid("af_rid"),
		af_rlast("af_rlast")

	{
		unsigned int i;

		SC_METHOD(gen_rst_n);
		sensitive << rst;

		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);

		for (i = 0; i < (sizeof dma / sizeof dma[0]); i++) {
			char name[16];

			snprintf(name, sizeof name, "demodma%d", i);
			dma[i] = new demodma(name);
		}

		bus.memmap(0xa0000000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debug.socket);

		for (i = 0; i < (sizeof dma / sizeof dma[0]); i++) {
			bus.memmap(0xa0010000ULL + 0x100 * i, 0x18 - 1,
				ADDRMODE_RELATIVE, -1, dma[i]->tgt_socket);
		}

		tlm2apb_tmr = new tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32> ("tlm2apb-tmr-bridge");
		bus.memmap(0xa0020000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, tlm2apb_tmr->tgt_socket);

#ifdef HAVE_VERILOG
		tlm2axi_al = new tlm2axilite_bridge<4, 32> ("tlm2axi-al-bridge");
		tlm2axi_af = new tlm2axi_bridge<10, AXIFULL_DATA_WIDTH> ("tlm2axi-af-bridge");
		bus.memmap(0xa0450000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, tlm2axi_al->tgt_socket);
		bus.memmap(0xa0460000ULL, 0x400 - 1,
				ADDRMODE_RELATIVE, -1, tlm2axi_af->tgt_socket);
#else
		bus.memmap(0xa0450000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, mem_al.socket);
		bus.memmap(0xa0460000ULL, 0x400 - 1,
				ADDRMODE_RELATIVE, -1, mem_af.socket);
#endif
		bus.memmap(0xa0800000ULL, 64 * 1024 - 1,
				ADDRMODE_RELATIVE, -1, mem.socket);

		bus.memmap(0x0LL, 0xffffffff - 1,
				ADDRMODE_RELATIVE, -1, *(zynq.s_axi_hpc_fpd[0]));

		zynq.s_axi_hpm_fpd[0]->bind(*(bus.t_sk[0]));

		for (i = 0; i < (sizeof dma / sizeof dma[0]); i++) {
			dma[i]->init_socket.bind(*(bus.t_sk[1 + i]));
			dma[i]->irq(zynq.pl2ps_irq[1 + i]);
		}

		debug.irq(zynq.pl2ps_irq[0]);

#ifdef HAVE_VERILOG
		/* Slow clock to keep simulation fast.  */
		clk = new sc_clock("clk", sc_time(10, SC_US));
#ifdef HAVE_VERILOG_VCS
		apb_timer = new apb_slave_timer("apb_timer");
#elif defined(HAVE_VERILOG_VERILATOR)
		apb_timer = new Vapb_timer("apb_timer");
		al = new Vaxilite_dev("axilite-dev");
		af = new Vaxifull_dev("axifull-dev");
#endif
                apb_timer->clk(*clk);
                apb_timer->rst(rst);
                apb_timer->irq(irq_tmr);
                apb_timer->psel(apbsig_timer_psel);
                apb_timer->penable(apbsig_timer_penable);
                apb_timer->pwrite(apbsig_timer_pwrite);
                apb_timer->paddr(apbsig_timer_paddr);
                apb_timer->pwdata(apbsig_timer_pwdata);
                apb_timer->prdata(apbsig_timer_prdata);
                apb_timer->pready(apbsig_timer_pready);

                tlm2apb_tmr->clk(*clk);

		checker.clk(*clk);
		checker.resetn(rst_n);

		checker.awvalid(al_awvalid);
		checker.awready(al_awready);
		checker.awaddr(al_awaddr);
		checker.awprot(al_awprot);

		checker.wvalid(al_wvalid);
		checker.wready(al_wready);
		checker.wdata(al_wdata);
		checker.wstrb(al_wstrb);

		checker.bvalid(al_bvalid);
		checker.bready(al_bready);
		checker.bresp(al_bresp);

		checker.arvalid(al_arvalid);
		checker.arready(al_arready);
		checker.araddr(al_araddr);
		checker.arprot(al_arprot);

		checker.rvalid(al_rvalid);
		checker.rready(al_rready);
		checker.rdata(al_rdata);
		checker.rresp(al_rresp);

		al->s00_axi_aclk(*clk);
		al->s00_axi_aresetn(rst_n);

		al->s00_axi_awvalid(al_awvalid);
		al->s00_axi_awready(al_awready);
		al->s00_axi_awaddr(al_awaddr);
		al->s00_axi_awprot(al_awprot);

		al->s00_axi_wvalid(al_wvalid);
		al->s00_axi_wready(al_wready);
		al->s00_axi_wdata(al_wdata);
		al->s00_axi_wstrb(al_wstrb);

		al->s00_axi_bvalid(al_bvalid);
		al->s00_axi_bready(al_bready);
		al->s00_axi_bresp(al_bresp);

		al->s00_axi_arvalid(al_arvalid);
		al->s00_axi_arready(al_arready);
		al->s00_axi_araddr(al_araddr);
		al->s00_axi_arprot(al_arprot);

		al->s00_axi_rvalid(al_rvalid);
		al->s00_axi_rready(al_rready);
		al->s00_axi_rdata(al_rdata);
		al->s00_axi_rresp(al_rresp);

		af->s00_axi_aclk(*clk);
		af->s00_axi_aresetn(rst_n);

		af->s00_axi_awvalid(af_awvalid);
		af->s00_axi_awready(af_awready);
		af->s00_axi_awaddr(af_awaddr);
		af->s00_axi_awprot(af_awprot);
		af->s00_axi_awuser(af_awuser);
		af->s00_axi_awregion(af_awregion);
		af->s00_axi_awqos(af_awqos);
		af->s00_axi_awcache(af_awcache);
		af->s00_axi_awburst(af_awburst);
		af->s00_axi_awsize(af_awsize);
		af->s00_axi_awlen(af_awlen);
		af->s00_axi_awid(af_awid);
		af->s00_axi_awlock(af_awlock);

		af->s00_axi_wvalid(af_wvalid);
		af->s00_axi_wready(af_wready);
		af->s00_axi_wdata(af_wdata);
		af->s00_axi_wstrb(af_wstrb);
		af->s00_axi_wuser(af_wuser);
		af->s00_axi_wlast(af_wlast);

		af->s00_axi_bvalid(af_bvalid);
		af->s00_axi_bready(af_bready);
		af->s00_axi_bresp(af_bresp);
		af->s00_axi_buser(af_buser);
		af->s00_axi_bid(af_bid);

		af->s00_axi_arvalid(af_arvalid);
		af->s00_axi_arready(af_arready);
		af->s00_axi_araddr(af_araddr);
		af->s00_axi_arprot(af_arprot);
		af->s00_axi_aruser(af_aruser);
		af->s00_axi_arregion(af_arregion);
		af->s00_axi_arqos(af_arqos);
		af->s00_axi_arcache(af_arcache);
		af->s00_axi_arburst(af_arburst);
		af->s00_axi_arsize(af_arsize);
		af->s00_axi_arlen(af_arlen);
		af->s00_axi_arid(af_arid);
		af->s00_axi_arlock(af_arlock);

		af->s00_axi_rvalid(af_rvalid);
		af->s00_axi_rready(af_rready);
		af->s00_axi_rdata(af_rdata);
		af->s00_axi_rresp(af_rresp);
		af->s00_axi_ruser(af_ruser);
		af->s00_axi_rid(af_rid);
		af->s00_axi_rlast(af_rlast);

		tlm2axi_al->clk(*clk);
		tlm2axi_af->clk(*clk);

		tlm2axi_al->resetn(rst_n);
		tlm2axi_af->resetn(rst_n);

		tlm2axi_al->awvalid(al_awvalid);
		tlm2axi_al->awready(al_awready);
		tlm2axi_al->awaddr(al_awaddr);
		tlm2axi_al->awprot(al_awprot);

		tlm2axi_al->wvalid(al_wvalid);
		tlm2axi_al->wready(al_wready);
		tlm2axi_al->wdata(al_wdata);
		tlm2axi_al->wstrb(al_wstrb);

		tlm2axi_al->bvalid(al_bvalid);
		tlm2axi_al->bready(al_bready);
		tlm2axi_al->bresp(al_bresp);

		tlm2axi_al->arvalid(al_arvalid);
		tlm2axi_al->arready(al_arready);
		tlm2axi_al->araddr(al_araddr);
		tlm2axi_al->arprot(al_arprot);

		tlm2axi_al->rvalid(al_rvalid);
		tlm2axi_al->rready(al_rready);
		tlm2axi_al->rdata(al_rdata);
		tlm2axi_al->rresp(al_rresp);

		/* AXI FULL.  */
		tlm2axi_af->awvalid(af_awvalid);
		tlm2axi_af->awready(af_awready);
		tlm2axi_af->awaddr(af_awaddr);
		tlm2axi_af->awprot(af_awprot);
		tlm2axi_af->awuser(af_awuser);
		tlm2axi_af->awregion(af_awregion);
		tlm2axi_af->awqos(af_awqos);
		tlm2axi_af->awcache(af_awcache);
		tlm2axi_af->awburst(af_awburst);
		tlm2axi_af->awsize(af_awsize);
		tlm2axi_af->awlen(af_awlen);
		tlm2axi_af->awid(af_awid);
		tlm2axi_af->awlock(af_awlock);

		tlm2axi_af->wvalid(af_wvalid);
		tlm2axi_af->wready(af_wready);
		tlm2axi_af->wdata(af_wdata);
		tlm2axi_af->wstrb(af_wstrb);
		tlm2axi_af->wuser(af_wuser);
		tlm2axi_af->wlast(af_wlast);

		tlm2axi_af->bvalid(af_bvalid);
		tlm2axi_af->bready(af_bready);
		tlm2axi_af->bresp(af_bresp);
		tlm2axi_af->bid(af_bid);
		tlm2axi_af->buser(af_buser);

		tlm2axi_af->arvalid(af_arvalid);
		tlm2axi_af->arready(af_arready);
		tlm2axi_af->araddr(af_araddr);
		tlm2axi_af->arprot(af_arprot);
		tlm2axi_af->aruser(af_aruser);
		tlm2axi_af->arregion(af_arregion);
		tlm2axi_af->arqos(af_arqos);
		tlm2axi_af->arcache(af_arcache);
		tlm2axi_af->arburst(af_arburst);
		tlm2axi_af->arsize(af_arsize);
		tlm2axi_af->arlen(af_arlen);
		tlm2axi_af->arid(af_arid);
		tlm2axi_af->arlock(af_arlock);

		tlm2axi_af->rvalid(af_rvalid);
		tlm2axi_af->rready(af_rready);
		tlm2axi_af->rdata(af_rdata);
		tlm2axi_af->rresp(af_rresp);
		tlm2axi_af->ruser(af_ruser);
		tlm2axi_af->rid(af_rid);
		tlm2axi_af->rlast(af_rlast);

#else
		clk = new sc_clock("clk", sc_time(20, SC_US));
                apbsig_timer_prdata = 0xeddebeef;
                apbsig_timer_pready = true;
                tlm2apb_tmr->clk(*clk);
#endif

                tlm2apb_tmr->psel(apbsig_timer_psel);
                tlm2apb_tmr->penable(apbsig_timer_penable);
                tlm2apb_tmr->pwrite(apbsig_timer_pwrite);
                tlm2apb_tmr->paddr(apbsig_timer_paddr);
                tlm2apb_tmr->pwdata(apbsig_timer_pwdata);
                tlm2apb_tmr->prdata(apbsig_timer_prdata);
                tlm2apb_tmr->pready(apbsig_timer_pready);

		zynq.tie_off();

		SC_THREAD(pull_reset);
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

#if defined(HAVE_VERILOG_VERILATOR) && VM_TRACE
        Verilated::traceEverOn(true);
        // If verilator was invoked with --trace argument,
        // and if at run time passed the +trace argument, turn on tracing
        VerilatedVcdSc* tfp = NULL;
        const char* flag = Verilated::commandArgsPlusMatch("trace");
        if (flag && 0 == strcmp(flag, "+trace")) {
                tfp = new VerilatedVcdSc;
                top->apb_timer->trace(tfp, 99);
                top->al->trace(tfp, 99);
                top->af->trace(tfp, 99);
                tfp->open("vlt_dump.vcd");
        }
#endif

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}

#if defined(HAVE_VERILOG_VERILATOR) && VM_TRACE
        if (tfp) { tfp->close(); tfp = NULL; }
#endif
	return 0;
}
