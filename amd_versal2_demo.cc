/*
 * Top level of the amd_versal2 cosim example.
 *
 * Copyright (c) 2023 Advanced Micro Devices Inc.
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
#include "debugdev.h"
#include "soc/amd/amd-versal2/amd-versal2.h"
#include "memory.h"
#include "demo-dma.h"
#include "wiredev.h"
#include "wire-loopback.h"
#include "tlm-bridges/tlm2apb-bridge.h"

#define RAM_SIZE (2 * 1024 * 1024)

#define NR_MASTERS	6
#define NR_DEVICES	11

SC_MODULE(Top)
{
	iconnect<NR_MASTERS, NR_DEVICES> bus;
	amd_versal2 versal2;
	debugdev debug;
	debugdev debug2;
	demodma dma;

	memory mem0;

	wiredev wreg_out_en_emio0;
	wiredev wreg_out_en_emio1;
	wiredev wreg_out_emio2;
	wiredev wreg_out_en_emio2;
	wire_loopback wlb;
	wiredev wreg_pl_reset;

	tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32> *tlm2apb_tmr;

	sc_signal<bool> apbsig_timer_psel;
	sc_signal<bool> apbsig_timer_penable;
	sc_signal<bool> apbsig_timer_pwrite;
	sc_signal<bool> apbsig_timer_pready;
	sc_signal<sc_bv<16> > apbsig_timer_paddr;
	sc_signal<sc_bv<32> > apbsig_timer_pwdata;
	sc_signal<sc_bv<32> > apbsig_timer_prdata;

	sc_clock *clk;
	sc_signal<bool> rst;

	SC_HAS_PROCESS(Top);

	void pull_reset(void) {
		/* Pull the reset signal.  */
		rst.write(true);
		wait(1, SC_US);
		rst.write(false);
	}

	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		sc_module(name),
		bus("bus"),
		versal2("versal2", sk_descr),
		debug("debugdev"),
		debug2("debugdev2"),
		dma("dma"),
		mem0("mem0", sc_time(1, SC_NS), RAM_SIZE),
		wreg_out_en_emio0("wreg_out_en_emio0", 32),
		wreg_out_en_emio1("wreg_out_en_emio1", 32),
		wreg_out_emio2("wreg_out_emio2", 32),
		wreg_out_en_emio2("wreg_out_en_emio2", 32),
		wlb("wlb", 32),
		wreg_pl_reset("wreg_pl_reset", versal2.pl_reset.size()),
		apbsig_timer_psel("apbtimer_psel"),
		apbsig_timer_penable("apbtimer_penable"),
		apbsig_timer_pwrite("apbtimer_pwrite"),
		apbsig_timer_pready("apbtimer_pready"),
		apbsig_timer_paddr("apbtimer_paddr"),
		apbsig_timer_pwdata("apbtimer_pwdata"),
		apbsig_timer_prdata("apbtimer_prdata"),
		rst("rst")
	{
		unsigned int i;

		clk = new sc_clock("clk", sc_time(10, SC_US));
		tlm2apb_tmr = new tlm2apb_bridge<bool, sc_bv, 16, sc_bv, 32>
		  ("tlm2apb-tmr-bridge");

		m_qk.set_global_quantum(quantum);

		versal2.rst(rst);

		bus.memmap(0x80000120ULL, 0xC - 1,
				ADDRMODE_RELATIVE, -1, wreg_pl_reset.socket);
		bus.memmap(0x80000000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debug.socket);
		bus.memmap(0x80020000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, tlm2apb_tmr->tgt_socket);
		bus.memmap(0x80000200ULL, 0x4 - 1,
			    ADDRMODE_RELATIVE, -1, wreg_out_emio2.socket);
		bus.memmap(0x80000300ULL, 0x4 - 1,
			    ADDRMODE_RELATIVE, -1, wreg_out_en_emio0.socket);
		bus.memmap(0x80000400ULL, 0x4 - 1,
			    ADDRMODE_RELATIVE, -1, wreg_out_en_emio1.socket);
		bus.memmap(0x80000500ULL, 0x4 - 1,
			    ADDRMODE_RELATIVE, -1, wreg_out_en_emio2.socket);
		bus.memmap(0xb0000000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debug2.socket);
		bus.memmap(0xb0010000ULL, 0x10 - 1,
				ADDRMODE_RELATIVE, -1, dma.tgt_socket);
		bus.memmap(0xb0020000ULL, RAM_SIZE - 1,
				ADDRMODE_RELATIVE, -1, mem0.socket);
		bus.memmap(0x0LL, UINT64_MAX - 1,
				ADDRMODE_RELATIVE, -1, *(versal2.s_pl_axi_fpd0));

		//
		// Bus masters
		//
		versal2.m_fpd_axi_pl->bind(*(bus.t_sk[0]));
		versal2.m_lpd_axi_pl->bind(*(bus.t_sk[1]));
		versal2.m_pmxc_axi_noc0->bind(*(bus.t_sk[2]));
		versal2.m_fpd_axi_noc0->bind(*(bus.t_sk[3]));
		versal2.m_mmu_noc0->bind(*(bus.t_sk[4]));

		dma.init_socket.bind(*(bus.t_sk[5]));

		/* Connect the PL irqs to the irq_pl_to_ps wires.  */
		debug.irq[0](versal2.pl2ps_irq[0]);
		debug.irq[1](versal2.npi_irq[0]);
		dma.irq(versal2.pl2ps_irq[1]);

		/* Connect EMIO0 to EMIO1, so outputs / inputs can be
		 * tested.  */
		for (i = 0; i < 32; i++) {
			wlb.wire_in[i](versal2.emio[0]->out[i]);
			wlb.wire_out[i](versal2.emio[1]->in[i]);
		}

		/* Connect EMIO2 outputs.  */
		for (i = 0; i < 32; i++) {
			wreg_out_emio2.wires_in[i](versal2.emio[2]->out[i]);
		}

		/* Connect output enable signals.  */
		for (i = 0; i < 32; i++) {
			wreg_out_en_emio0.wires_in[i](
			        versal2.emio[0]->out_enable[i]);
			wreg_out_en_emio1.wires_in[i](
			        versal2.emio[1]->out_enable[i]);
			wreg_out_en_emio2.wires_in[i](
			        versal2.emio[2]->out_enable[i]);
		}

		for(i = 0; i < versal2.pl_reset.size(); i++) {
			wreg_pl_reset.wires_in[i](versal2.pl_reset[i]);
		}

		//
		// Dummy timer for now
		//
		clk = new sc_clock("clk", sc_time(20, SC_US));
		apbsig_timer_prdata = 0xeddebeef;
		apbsig_timer_pready = true;
		tlm2apb_tmr->clk(*clk);
		tlm2apb_tmr->psel(apbsig_timer_psel);
		tlm2apb_tmr->penable(apbsig_timer_penable);
		tlm2apb_tmr->pwrite(apbsig_timer_pwrite);
		tlm2apb_tmr->paddr(apbsig_timer_paddr);
		tlm2apb_tmr->pwdata(apbsig_timer_pwdata);
		tlm2apb_tmr->prdata(apbsig_timer_prdata);
		tlm2apb_tmr->pready(apbsig_timer_pready);

		/* Tie off any remaining unconnected signals.  */
		versal2.tie_off();

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

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}
	return 0;
}
