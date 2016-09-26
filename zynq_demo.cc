/*
 * Top level of the Zynq cosim example.
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
#include "debugdev.h"
#include "xilinx-zynq.h"

#define NR_MASTERS	1
#define NR_DEVICES	1

SC_MODULE(Top)
{
	iconnect<NR_MASTERS, NR_DEVICES>	*bus;
	xilinx_zynq zynq;
	debugdev *debug;
	sc_signal<bool> rst;


	Top(sc_module_name name, const char *sk_descr, sc_time quantum) :
		zynq("zynq", sk_descr),
		rst("rst")
	{
		m_qk.set_global_quantum(quantum);

		zynq.rst(rst);

		bus   = new iconnect<NR_MASTERS, NR_DEVICES> ("bus");
		debug = new debugdev("debug");

		bus->memmap(0x40000000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debug->socket);

		zynq.m_axi_gp[0]->bind(*(bus->t_sk[0]));

		/* Connect the PL irqs to the irq_pl_to_ps wires.  */
		debug->irq(zynq.pl2ps_irq[0]);

		/* Tie off any remaining unconnected signals.  */
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

	/* Pull the reset signal.  */
	top->rst.write(true);
	sc_start(1, SC_US);
	top->rst.write(false);

	sc_start();
	if (trace_fp) {
		sc_close_vcd_trace_file(trace_fp);
	}
	return 0;
}
