/*
 * Top level of the Versal Net CDx stub cosim example.
 *
 * Copyright (c) 2022 Xilinx Inc.
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
#include "soc/xilinx/versal-net/xilinx-versal-net.h"
#include "soc/dma/xilinx-cdma.h"
#include "tlm-extensions/genattr.h"
#include "memory.h"

#define RAM_SIZE (2 * 1024 * 1024)

#define NR_MASTERS	3
#define NR_DEVICES	7

class CounterDev : public sc_core::sc_module
{
public:
	enum {
		R_COUNTER = 0,
	};

	tlm_utils::simple_target_socket<CounterDev> tgt_socket;

	CounterDev(sc_core::sc_module_name name) :
		sc_module(name),
		tgt_socket("tgt-socket"),
		r_counter(0)
	{
		tgt_socket.register_b_transport(this, &CounterDev::b_transport);
	}

private:
	virtual void b_transport(tlm::tlm_generic_payload& trans,
			sc_time& delay)
	{
		tlm::tlm_command cmd = trans.get_command();
		unsigned char *data = trans.get_data_ptr();
		unsigned int len = trans.get_data_length();
		uint64_t addr = trans.get_address();

		if (len != 4 || trans.get_byte_enable_ptr() ||
			addr != R_COUNTER) {
			trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
			return;
		}

		if (trans.get_command() == tlm::TLM_READ_COMMAND) {
			memcpy(reinterpret_cast<void*>(data),
				reinterpret_cast<const void *>(&r_counter),
				len);

			r_counter++;

		} else if (cmd == tlm::TLM_WRITE_COMMAND) {
			memcpy(reinterpret_cast<void*>(&r_counter),
				reinterpret_cast<const void *>(data),
				len);
		}
	}

	uint32_t r_counter;
};

class SMIDdev : public sc_core::sc_module
{
public:
	tlm_utils::simple_target_socket<SMIDdev> tgt_socket;
	tlm_utils::simple_initiator_socket<SMIDdev> init_socket;

	SMIDdev(sc_core::sc_module_name name, uint32_t smid) :
		sc_module(name),
		tgt_socket("tgt-socket"),
		init_socket("init-socket"),
		m_smid(smid)
	{
		tgt_socket.register_b_transport(this, &SMIDdev::b_transport);
	}

private:
	virtual void b_transport(tlm::tlm_generic_payload& trans,
			sc_time& delay)
	{
		genattr_extension *genattr;

		trans.get_extension(genattr);
		if (!genattr) {
			genattr = new genattr_extension();
			trans.set_extension(genattr);
		}

		//
		// Setup the SMID (master_id)
		//
		genattr->set_master_id(m_smid);

		init_socket->b_transport(trans, delay);
	}

	uint32_t m_smid;
};

SC_MODULE(Top)
{
	iconnect<NR_MASTERS, NR_DEVICES> bus;
	xilinx_versal_net versal_net;
	debugdev debugdev_cpm;

	memory mem0;
	memory mem1;

	xilinx_cdma cdma0;
	SMIDdev smid_cdma0;

	xilinx_cdma cdma1;
	SMIDdev smid_cdma1;

	CounterDev counter;
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
		versal_net("versal-net", sk_descr),
		debugdev_cpm("debugdev-cpm"),

		mem0("mem0", sc_time(1, SC_NS), RAM_SIZE),
		mem1("mem1", sc_time(1, SC_NS), RAM_SIZE),

		cdma0("cdma0"),
		smid_cdma0("smid-cdma0", 0x250),

		cdma1("cdma1"),
		smid_cdma1("smid-cdma1", 0x251),

		counter("counter"),
		rst("rst")
	{
		m_qk.set_global_quantum(quantum);

		versal_net.rst(rst);

		//
		// Bus slave devices
		//
		// Address         Device
		// [0xe4000000] : debugdev
		// [0xe4020000] : CDMA 0 (SMID 0x250)
		// [0xe4030000] : CDMA 1 (SMID 0x251)
		// [0xe4040000] : counter
		// [0xe4100000] : Memory 2 MB
		// [0xe4300000] : Memory 2 MB
		//
		bus.memmap(0xe4000000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, debugdev_cpm.socket);
		bus.memmap(0xe4020000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, cdma0.target_socket);
		bus.memmap(0xe4030000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, cdma1.target_socket);
		bus.memmap(0xe4040000ULL, 0x100 - 1,
				ADDRMODE_RELATIVE, -1, counter.tgt_socket);
		bus.memmap(0xe4100000ULL, RAM_SIZE - 1,
				ADDRMODE_RELATIVE, -1, mem0.socket);
		bus.memmap(0xe4300000ULL, RAM_SIZE - 1,
				ADDRMODE_RELATIVE, -1, mem1.socket);
		bus.memmap(0x0LL, UINT64_MAX,
				ADDRMODE_RELATIVE, -1, *(versal_net.s_cpm));

		//
		// Bus masters
		//
		versal_net.m_cpm->bind(*(bus.t_sk[0]));
		smid_cdma0.init_socket(*(bus.t_sk[1]));
		smid_cdma1.init_socket(*(bus.t_sk[2]));

		//
		// CDMA0 SMID setup
		//
		cdma0.init_socket(smid_cdma0.tgt_socket);

		//
		// CDMA1 SMID setup
		//
		cdma1.init_socket(smid_cdma1.tgt_socket);

		/* Connect the PL irqs to the irq_pl_to_ps wires.  */
		debugdev_cpm.irq(versal_net.pl2ps_irq[0]);

		/* Tie off any remaining unconnected signals.  */
		versal_net.tie_off();

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
