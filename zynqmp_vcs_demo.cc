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
#define HAVE_VERILOG
#define HAVE_VERILOG_VCS
#include "axi_ram.h"
#include "iconnect.h"
#include "debugdev.h"
#include "demo-dma.h"
#include "xilinx-zynqmp.h"

#include "tlm-bridges/tlm2axi-bridge.h"

#define NR_MASTERS	2
#define NR_DEVICES	4

#ifndef BASE_ADDR
#define BASE_ADDR 0x28000000ULL
#endif

#define ID_WIDTH 8
#define ADDR_WIDTH 8
#define DATA_WIDTH 32
#define STRB_WIDTH 4
SC_MODULE(Top)
{
	SC_HAS_PROCESS(Top);
	iconnect<NR_MASTERS, NR_DEVICES>	*bus;
	xilinx_zynqmp zynq;
	debugdev *debug;
	demodma *dma;

	sc_signal<bool> rst, rst_n;

	sc_clock *clk;
	
	tlm2axi_bridge<ADDR_WIDTH, DATA_WIDTH> *tlm2axi_bridge_inst;
	
	axi_ram *axi_ram_inst;


    sc_signal<sc_bv<ID_WIDTH> >     s_axi_awid;
    sc_signal<sc_bv<ADDR_WIDTH> >   s_axi_awaddr;
    sc_signal<sc_bv<8> >            s_axi_awlen;
    sc_signal<sc_bv<3> >            s_axi_awsize;
    sc_signal<sc_bv<2> >            s_axi_awburst;
    sc_signal<bool>                s_axi_awlock;
    sc_signal<sc_bv<4> >            s_axi_awcache;
    sc_signal<sc_bv<3> >            s_axi_awprot;
    sc_signal<bool>                s_axi_awvalid;
    sc_signal<bool>                s_axi_awready;
    sc_signal<sc_bv<DATA_WIDTH> >   s_axi_wdata;
    sc_signal<sc_bv<STRB_WIDTH> >   s_axi_wstrb;
    sc_signal<bool>                s_axi_wlast;
    sc_signal<bool>                s_axi_wvalid;
    sc_signal<bool>                s_axi_wready;
    sc_signal<sc_bv<ID_WIDTH> >     s_axi_bid;
    sc_signal<sc_bv<2> >            s_axi_bresp;
    sc_signal<bool>                s_axi_bvalid;
    sc_signal<bool>                s_axi_bready;
    sc_signal<sc_bv<ID_WIDTH> >     s_axi_arid;
    sc_signal<sc_bv<ADDR_WIDTH> >   s_axi_araddr;
    sc_signal<sc_bv<8> >            s_axi_arlen;
    sc_signal<sc_bv<3> >            s_axi_arsize;
    sc_signal<sc_bv<2> >            s_axi_arburst;
    sc_signal<bool>                s_axi_arlock;
    sc_signal<sc_bv<4> >            s_axi_arcache;
    sc_signal<sc_bv<3> >            s_axi_arprot;
    sc_signal<bool>                s_axi_arvalid;
    sc_signal<bool>                s_axi_arready;
    sc_signal<sc_bv<ID_WIDTH> >     s_axi_rid;
    sc_signal<sc_bv<DATA_WIDTH> >   s_axi_rdata;
    sc_signal<sc_bv<2> >            s_axi_rresp;
    sc_signal<bool>                s_axi_rlast;
    sc_signal<bool>                s_axi_rvalid;
    sc_signal<bool>                s_axi_rready;
    sc_signal<sc_bv<2> >            s_axi_ruser;
    sc_signal<sc_bv<2> >            s_axi_awuser;
    sc_signal<sc_bv<2> >            s_axi_wuser;
    sc_signal<sc_bv<2> >            s_axi_aruser;
    sc_signal<sc_bv<2> >            s_axi_buser;
    sc_signal<sc_bv<4> >            s_axi_arqos;
    sc_signal<sc_bv<4> >            s_axi_awqos;
    sc_signal<sc_bv<4> >            s_axi_arregion;
    sc_signal<sc_bv<4> >            s_axi_awregion;

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		zynq("zynq", sk_descr),
		rst("rst"),
		rst_n("rst_n"),
		
        s_axi_awid("cosim_s_axi_awid"),
        s_axi_awaddr("cosim_s_axi_awaddr"),
        s_axi_awlen("cosim_s_axi_awlen"),
        s_axi_awsize("cosim_s_axi_awsize"),
        s_axi_awburst("cosim_s_axi_awburst"),
        s_axi_awlock("cosim_s_axi_awlock"),
        s_axi_awcache("cosim_s_axi_awcache"),
        s_axi_awprot("cosim_s_axi_awprot"),
        s_axi_awvalid("cosim_s_axi_awvalid"),
        s_axi_awready("cosim_s_axi_awready"),
        s_axi_wdata("cosim_s_axi_wdata"),
        s_axi_wstrb("cosim_s_axi_wstrb"),
        s_axi_wlast("cosim_s_axi_wlast"),
        s_axi_wvalid("cosim_s_axi_wvalid"),
        s_axi_wready("cosim_s_axi_wready"),
        s_axi_bid("cosim_s_axi_bid"),
        s_axi_bresp("cosim_s_axi_bresp"),
        s_axi_bvalid("cosim_s_axi_bvalid"),
        s_axi_bready("cosim_s_axi_bready"),
        s_axi_arid("cosim_s_axi_arid"),
        s_axi_araddr("cosim_s_axi_araddr"),
        s_axi_arlen("cosim_s_axi_arlen"),
        s_axi_arsize("cosim_s_axi_arsize"),
        s_axi_arburst("cosim_s_axi_arburst"),
        s_axi_arlock("cosim_s_axi_arlock"),
        s_axi_arcache("cosim_s_axi_arcache"),
        s_axi_arprot("cosim_s_axi_arprot"),
        s_axi_arvalid("cosim_s_axi_arvalid"),
        s_axi_arready("cosim_s_axi_arready"),
        s_axi_rid("cosim_s_axi_rid"),
        s_axi_rdata("cosim_s_axi_rdata"),
        s_axi_rresp("cosim_s_axi_rresp"),
        s_axi_rlast("cosim_s_axi_rlast"),
        s_axi_rvalid("cosim_s_axi_rvalid"),
        s_axi_rready("cosim_s_axi_rready"),
	s_axi_ruser("cosim_s_axi_ruser"),
	s_axi_awuser("cosim_s_axi_awuser"),
	s_axi_wuser("cosim_s_axi_wuser"),
	s_axi_aruser("cosim_s_axi_aruser"),
	s_axi_buser("cosim_s_axi_buser"),
	s_axi_awqos("cosim_s_axi_awqos"),
	s_axi_arqos("cosim_s_axi_arqos"),
	s_axi_arregion("cosim_s_axi_arregion"),
	s_axi_awregion("cosim_s_axi_awregion")
	{
        printf("initializing top design\n");
		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);
		bus   = new iconnect<NR_MASTERS, NR_DEVICES> ("bus");
		debug = new debugdev("debug");
		dma = new demodma("demodma");

		bus->memmap(BASE_ADDR, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debug->socket);
		bus->memmap(BASE_ADDR + 0x10000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, dma->tgt_socket);

		tlm2axi_bridge_inst = new tlm2axi_bridge<ADDR_WIDTH, DATA_WIDTH> ("tlm2axi-bridge-inst");
		
		bus->memmap(BASE_ADDR + 0x20000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, tlm2axi_bridge_inst->tgt_socket);

		bus->memmap(0x0LL, 0xffffffff - 1,
				ADDRMODE_RELATIVE, -1, *(zynq.s_axi_hpc_fpd[0]));

		zynq.s_axi_hpm_fpd[0]->bind(*(bus->t_sk[0]));

		dma->init_socket.bind(*(bus->t_sk[1]));

		debug->irq(zynq.pl2ps_irq[0]);
		dma->irq(zynq.pl2ps_irq[1]);

		/* Slow clock to keep simulation fast.  */
		clk = new sc_clock("clk", sc_time(10, SC_US));
		
		axi_ram_inst = new axi_ram("axi_ram_inst");
        axi_ram_inst->clk(*clk);
        axi_ram_inst->rst(rst);
        
        axi_ram_inst->s_axi_awid(s_axi_awid);
        axi_ram_inst->s_axi_awaddr(s_axi_awaddr);
        axi_ram_inst->s_axi_awlen(s_axi_awlen);
        axi_ram_inst->s_axi_awsize(s_axi_awsize);
        axi_ram_inst->s_axi_awburst(s_axi_awburst);
        axi_ram_inst->s_axi_awlock(s_axi_awlock);
        axi_ram_inst->s_axi_awcache(s_axi_awcache);
        axi_ram_inst->s_axi_awprot(s_axi_awprot);
        axi_ram_inst->s_axi_awvalid(s_axi_awvalid);
        axi_ram_inst->s_axi_awready(s_axi_awready);
        axi_ram_inst->s_axi_wdata(s_axi_wdata);
        axi_ram_inst->s_axi_wstrb(s_axi_wstrb);
        axi_ram_inst->s_axi_wlast(s_axi_wlast);
        axi_ram_inst->s_axi_wvalid(s_axi_wvalid);
        axi_ram_inst->s_axi_wready(s_axi_wready);
        axi_ram_inst->s_axi_bid(s_axi_bid);
        axi_ram_inst->s_axi_bresp(s_axi_bresp);
        axi_ram_inst->s_axi_bvalid(s_axi_bvalid);
        axi_ram_inst->s_axi_bready(s_axi_bready);
        axi_ram_inst->s_axi_arid(s_axi_arid);
        axi_ram_inst->s_axi_araddr(s_axi_araddr);
        axi_ram_inst->s_axi_arlen(s_axi_arlen);
        axi_ram_inst->s_axi_arsize(s_axi_arsize);
        axi_ram_inst->s_axi_arburst(s_axi_arburst);
        axi_ram_inst->s_axi_arlock(s_axi_arlock);
        axi_ram_inst->s_axi_arcache(s_axi_arcache);
        axi_ram_inst->s_axi_arprot(s_axi_arprot);
        axi_ram_inst->s_axi_arvalid(s_axi_arvalid);
        axi_ram_inst->s_axi_arready(s_axi_arready);
        axi_ram_inst->s_axi_rid(s_axi_rid);
        axi_ram_inst->s_axi_rdata(s_axi_rdata);
        axi_ram_inst->s_axi_rresp(s_axi_rresp);
        axi_ram_inst->s_axi_rlast(s_axi_rlast);
        axi_ram_inst->s_axi_rvalid(s_axi_rvalid);
        axi_ram_inst->s_axi_rready(s_axi_rready);

        tlm2axi_bridge_inst->clk(*clk);
	tlm2axi_bridge_inst->resetn(rst_n);

        tlm2axi_bridge_inst->awid(s_axi_awid);
        tlm2axi_bridge_inst->awaddr(s_axi_awaddr);
        tlm2axi_bridge_inst->awlen(s_axi_awlen);
        tlm2axi_bridge_inst->awsize(s_axi_awsize);
        tlm2axi_bridge_inst->awburst(s_axi_awburst);
        tlm2axi_bridge_inst->awlock(s_axi_awlock);
        tlm2axi_bridge_inst->awcache(s_axi_awcache);
        tlm2axi_bridge_inst->awprot(s_axi_awprot);
        tlm2axi_bridge_inst->awvalid(s_axi_awvalid);
        tlm2axi_bridge_inst->awready(s_axi_awready);
        tlm2axi_bridge_inst->wdata(s_axi_wdata);
        tlm2axi_bridge_inst->wstrb(s_axi_wstrb);
        tlm2axi_bridge_inst->wlast(s_axi_wlast);
        tlm2axi_bridge_inst->wvalid(s_axi_wvalid);
        tlm2axi_bridge_inst->wready(s_axi_wready);
        tlm2axi_bridge_inst->bid(s_axi_bid);
        tlm2axi_bridge_inst->bresp(s_axi_bresp);
        tlm2axi_bridge_inst->bvalid(s_axi_bvalid);
        tlm2axi_bridge_inst->bready(s_axi_bready);
        tlm2axi_bridge_inst->arid(s_axi_arid);
        tlm2axi_bridge_inst->araddr(s_axi_araddr);
        tlm2axi_bridge_inst->arlen(s_axi_arlen);
        tlm2axi_bridge_inst->arsize(s_axi_arsize);
        tlm2axi_bridge_inst->arburst(s_axi_arburst);
        tlm2axi_bridge_inst->arlock(s_axi_arlock);
        tlm2axi_bridge_inst->arcache(s_axi_arcache);
        tlm2axi_bridge_inst->arprot(s_axi_arprot);
        tlm2axi_bridge_inst->arvalid(s_axi_arvalid);
        tlm2axi_bridge_inst->arready(s_axi_arready);
        tlm2axi_bridge_inst->rid(s_axi_rid);
        tlm2axi_bridge_inst->rdata(s_axi_rdata);
        tlm2axi_bridge_inst->rresp(s_axi_rresp);
        tlm2axi_bridge_inst->rlast(s_axi_rlast);
        tlm2axi_bridge_inst->rvalid(s_axi_rvalid);
        tlm2axi_bridge_inst->rready(s_axi_rready);
        tlm2axi_bridge_inst->ruser(s_axi_ruser);
        tlm2axi_bridge_inst->aruser(s_axi_aruser);
        tlm2axi_bridge_inst->awuser(s_axi_awuser);
        tlm2axi_bridge_inst->wuser(s_axi_wuser);
        tlm2axi_bridge_inst->buser(s_axi_buser);
        tlm2axi_bridge_inst->arqos(s_axi_arqos);
        tlm2axi_bridge_inst->awqos(s_axi_awqos);
        tlm2axi_bridge_inst->awregion(s_axi_awregion);
        tlm2axi_bridge_inst->arregion(s_axi_arregion);
	
	s_axi_ruser = 0;
	s_axi_aruser = 0;
	s_axi_awuser = 0;
	s_axi_wuser = 0;
	s_axi_buser = 0;
	s_axi_arqos = 0;
	s_axi_awqos = 0;



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
    const char *socket_name;
	uint64_t sync_quantum;

//	if (argc < 3) {
		sync_quantum = 100000;
        	socket_name = "unix:/tmp/qemu/qemu-rport-_machine_cosim";
//	} else {
//		sync_quantum = strtoull(argv[2], NULL, 10);
 //       	socket_name = argv[1];
//	}

	sc_set_time_resolution(1, SC_FS);

	top = new Top("top", socket_name, sc_time((double) sync_quantum, SC_PS));

    printf("realy to pull the reset signal\n");
	/* Pull the reset signal.  */
	top->rst.write(true);
	top->rst_n.write(false);
    printf("start sc_start for reset\n");
	sc_start(1, SC_US);
	top->rst.write(false);
	top->rst_n.write(true);

    printf("formal starting the design\n");
	sc_start(10000000, SC_SEC);
    sc_stop();
	return -8;
}
