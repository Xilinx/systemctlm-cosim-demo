/*
 * A demo/debug device.
 *
 * Copyright (c) 2013 Xilinx Inc.
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

#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

using namespace sc_core;
using namespace std;

#include "debugdev.h"
#include <sys/types.h>
#include <time.h>

debugdev::debugdev(sc_module_name name)
	: sc_module(name), socket("socket")
{
	socket.register_b_transport(this, &debugdev::b_transport);
	socket.register_transport_dbg(this, &debugdev::transport_dbg);
}

void debugdev::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
{
	tlm::tlm_command cmd = trans.get_command();
	sc_dt::uint64 addr = trans.get_address();
	unsigned char *data = trans.get_data_ptr();
	unsigned int len = trans.get_data_length();
	unsigned char *byt = trans.get_byte_enable_ptr();
	unsigned int wid = trans.get_streaming_width();

	if (byt != 0) {
		trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
		return;
	}

	if (len > 4 || wid < len) {
		trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
		return;
	}
	trans.set_response_status(tlm::TLM_OK_RESPONSE);

	// Pretend this is slow!
	delay += sc_time(1, SC_US);

	if (trans.get_command() == tlm::TLM_READ_COMMAND) {
		sc_time now = sc_time_stamp() + delay;
		uint32_t v = 0;

		switch (addr) {
			case 0:
//				cout << "read " << addr << " " << t << endl;
				v = now.to_seconds() * 1000 * 1000 * 1000;
				break;
			case 0xc:
				v = irq.read();
				break;
			case 0x10:
				v = clock();
				break;
			case 0xf0:
				trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
				break;
			case 0xf4:
				trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
				break;
			default:
				break;
		}
		memcpy(data, &v, len);
	} else if (cmd == tlm::TLM_WRITE_COMMAND) {
		static sc_time old_ts = SC_ZERO_TIME, now, diff;

		now = sc_time_stamp() + delay;
		diff = now - old_ts;
		switch (addr) {
			case 0:
				cout << "TRACE: " << " "
				     << hex << * (uint32_t *) data
				     << " " << now << " diff=" << diff << "\n";
				break;
			case 0x4:
				putchar(* (uint32_t *) data);
				break;
			case 0x8:
				cout << "STOP: " << " "
				     << hex << * (uint32_t *) data
				     << " " << now << "\n";
				sc_stop();
				exit(1);
				break;
			case 0xc:
				irq.write(data[0] & 1);
				break;
			case 0xf0:
				trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
				break;
			case 0xf4:
				trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
				break;
			default:
				break;
		}
		old_ts = now;
	}

}

unsigned int debugdev::transport_dbg(tlm::tlm_generic_payload& trans)
{
	unsigned int len = trans.get_data_length();
	return len;
}
