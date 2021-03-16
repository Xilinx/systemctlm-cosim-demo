/*
 * A DMA device for demo purposes.
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

#include "demo-dma.h"
#include <sys/types.h>

demodma::demodma(sc_module_name name)
	: sc_module(name), tgt_socket("tgt-socket")
{
	tgt_socket.register_b_transport(this, &demodma::b_transport);
	memset(&regs, 0, sizeof regs);

	SC_THREAD(do_dma_copy);
	dont_initialize();
	sensitive << ev_dma_copy;
}

void demodma::do_dma_trans(tlm::tlm_command cmd, unsigned char *buf,
				sc_dt::uint64 addr, sc_dt::uint64 len)
{
	tlm::tlm_generic_payload tr;
	sc_time delay = SC_ZERO_TIME;

	tr.set_command(cmd);
	tr.set_address(addr);
	tr.set_data_ptr(buf);
	tr.set_data_length(len);
	tr.set_streaming_width(len);
	tr.set_dmi_allowed(false);
	tr.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

	if (regs.byte_en) {
		tr.set_byte_enable_ptr((unsigned char *) &regs.byte_en);
		tr.set_byte_enable_length(sizeof regs.byte_en);
	}

	init_socket->b_transport(tr, delay);

	switch (tr.get_response_status()) {
	case tlm::TLM_OK_RESPONSE:
		regs.error_resp = DEMODMA_RESP_OKAY;
		break;
	case tlm::TLM_ADDRESS_ERROR_RESPONSE:
		printf("%s:%d DMA transaction error!\n", __func__, __LINE__);
		regs.error_resp = DEMODMA_RESP_ADDR_DECODE_ERROR;
		break;
	default:
		printf("%s:%d DMA transaction error!\n", __func__, __LINE__);
		regs.error_resp = DEMODMA_RESP_BUS_GENERIC_ERROR;
		break;
	}
}

void demodma::update_irqs(void)
{
	irq.write(regs.ctrl & DEMODMA_CTRL_DONE);
}

void demodma::do_dma_copy(void)
{
	unsigned char buf[32];

	while (true) {
		if (!(regs.ctrl & DEMODMA_CTRL_RUN)) {
			wait(ev_dma_copy);
		}

		if (regs.len > 0 && regs.ctrl & DEMODMA_CTRL_RUN) {
			unsigned int tlen = regs.len > sizeof buf ? sizeof buf : regs.len;

			do_dma_trans(tlm::TLM_READ_COMMAND, buf, regs.src_addr, tlen);
			do_dma_trans(tlm::TLM_WRITE_COMMAND, buf, regs.dst_addr, tlen);

			regs.dst_addr += tlen;
			regs.src_addr += tlen;
			regs.len -= tlen;
		}

		if (regs.len == 0 && regs.ctrl & DEMODMA_CTRL_RUN) {
			regs.ctrl &= ~DEMODMA_CTRL_RUN;
			/* If the DMA was running, signal done.  */
			regs.ctrl |= DEMODMA_CTRL_DONE;
		} else {
			// Artificial delay between bursts.
			wait(sc_time(1, SC_US));
		}
		update_irqs();
	}
}

void demodma::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
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

	addr >>= 2;
	addr &= 7;
	if (trans.get_command() == tlm::TLM_READ_COMMAND) {
		memcpy(data, &regs.u32[addr], len);
	} else if (cmd == tlm::TLM_WRITE_COMMAND) {
		unsigned char buf[4];
		memcpy(&regs.u32[addr], data, len);
		switch (addr) {
			case 3:
				// speculative read for testing inline path.
				do_dma_trans(tlm::TLM_READ_COMMAND, buf, regs.src_addr, 4);
				/* The dma copies after a usec.  */
				ev_dma_copy.notify(delay + sc_time(1, SC_US));
				break;
			default:
				/* No side-effect.  */
				break;
		}
	}
	trans.set_response_status(tlm::TLM_OK_RESPONSE);
}
