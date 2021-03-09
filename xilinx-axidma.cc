/*
 * Partial model of the Xilinx AXI DMA.
 * We only support Direct Register Mmode.
 *
 * Copyright (c) 2015 Xilinx Inc.
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

#include "tlm-extensions/genattr.h"
#include "xilinx-axidma.h"
#include <sys/types.h>

#define DEBUG_DMA 0
#define D(x) do {			\
		if (DEBUG_DMA) {	\
			x;		\
		}			\
	} while (0)

axidma_mm2s::axidma_mm2s(sc_module_name name, bool use_memcpy)
	: axidma(name, use_memcpy), stream_socket("stream-socket")
{
}

axidma_s2mm::axidma_s2mm(sc_module_name name, bool use_memcpy)
	: axidma(name, use_memcpy), stream_socket("stream-socket")
{
	stream_socket.register_b_transport(this, &axidma_s2mm::s_b_transport);
}

axidma::axidma(sc_module_name name, bool use_memcpy)
	: sc_module(name), tgt_socket("tgt-socket"), irq("irq"),
	  use_memcpy(use_memcpy)
{
	tgt_socket.register_b_transport(this, &axidma::b_transport);
	memset(&regs, 0, sizeof regs);

	SC_METHOD(update_irqs);
	dont_initialize();
	sensitive << ev_update_irqs;
	SC_THREAD(do_dma_copy);
}

void axidma::do_dma_trans(tlm::tlm_command cmd, unsigned char *buf,
				sc_dt::uint64 addr, sc_dt::uint64 len,
				sc_time &delay)
{
	tlm::tlm_generic_payload tr;

	tr.set_command(cmd);
	tr.set_address(addr);
	tr.set_data_ptr(buf);
	tr.set_data_length(len);
	tr.set_streaming_width(len);
	tr.set_dmi_allowed(false);
	tr.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

	init_socket->b_transport(tr, delay);
	if (tr.get_response_status() != tlm::TLM_OK_RESPONSE) {
		printf("%s:%d DMA transaction error!\n", __func__, __LINE__);
	}
}

void axidma_mm2s::do_stream_trans(tlm::tlm_command cmd, unsigned char *buf,
				sc_dt::uint64 addr, sc_dt::uint64 len, bool eop,
				sc_time &delay)
{
	tlm::tlm_generic_payload tr;
	genattr_extension *genattr = new genattr_extension();

	tr.set_command(cmd);
	tr.set_address(addr);
	tr.set_data_ptr(buf);
	tr.set_data_length(len);
	tr.set_streaming_width(len);
	tr.set_dmi_allowed(false);
	tr.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

	genattr->set_eop(eop);
	tr.set_extension(genattr);

	stream_socket->b_transport(tr, delay);
	if (tr.get_response_status() != tlm::TLM_OK_RESPONSE) {
		printf("%s:%d DMA transaction error!\n", __func__, __LINE__);
	}

	tr.release_extension(genattr);
}

void axidma::update_irqs(void)
{
	D(printf("DMA irq=%d\n", regs.sr & regs.cr & AXIDMA_CR_IOC_IRQ_EN));
	irq.write(regs.sr & regs.cr & AXIDMA_CR_IOC_IRQ_EN);
}

void axidma_s2mm::do_dma_copy(void) {}

void axidma_mm2s::do_dma_copy(void)
{
	while (1) {
		unsigned char buf[2 * 1024];
		uint64_t addr;
		sc_time delay = SC_ZERO_TIME;
		unsigned int tlen;
		bool eop;

		if (!regs.length) {
			wait(ev_dma_copy);
		}

		assert(!(regs.sr & AXIDMA_SR_IDLE));
		tlen = regs.length > sizeof buf ? sizeof buf : regs.length;
		eop = tlen == regs.length;

		addr = regs.addr_msb;
		addr <<= 32;
		addr += regs.addr;

		if (use_memcpy) {
			memcpy(buf, (void *) addr, tlen);
		} else {
			do_dma_trans(tlm::TLM_READ_COMMAND, buf, addr, tlen, delay);
		}
		do_stream_trans(tlm::TLM_WRITE_COMMAND, buf, addr, tlen, eop, delay);

		addr += tlen;
		regs.length -= tlen;

		regs.addr = addr;
		regs.addr_msb = addr >> 32;

		if (regs.length == 0) {
			/* If the DMA was running, signal done.  */
			regs.sr |= AXIDMA_SR_IDLE | AXIDMA_SR_IOC_IRQ;
			ev_update_irqs.notify();
		}
	}
}

void axidma::b_transport(tlm::tlm_generic_payload& trans, sc_time& delay)
{
	tlm::tlm_command cmd = trans.get_command();
	sc_dt::uint64    addr = trans.get_address();
	unsigned char*   data = trans.get_data_ptr();
	unsigned int     len = trans.get_data_length();
	unsigned char*   byt = trans.get_byte_enable_ptr();
	unsigned int     wid = trans.get_streaming_width();

	if (byt != 0) {
		trans.set_response_status(tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE);
		return;
	}

	if (len > 4 || wid < len) {
		trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
		return;
	}

	addr >>= 2;
	if (trans.get_command() == tlm::TLM_READ_COMMAND) {
		memcpy(data, &regs.u32[addr], len);
	} else if (cmd == tlm::TLM_WRITE_COMMAND) {
		uint32_t v;
		memcpy(&v, data, len);
		switch (addr) {
		case AXIDMA_R_SR:
			regs.u32[addr] &= ~(v & AXIDMA_SR_IOC_IRQ);
			D(printf("%s: SR=%x.%x val=%x\n", name(),
				regs.sr, regs.u32[addr], v));
			break;
		case AXIDMA_R_LENGTH:
			length_copied = 0;
			regs.length = v;
			regs.sr &= ~(AXIDMA_SR_IDLE);
			D(printf("%s: write LENGTH %d\n",
				name(), regs.length));
			ev_dma_copy.notify();
			break;
		default:
			/* No side-effect.  */
			regs.u32[addr] = v;
			break;
		}
	}
	ev_update_irqs.notify();
	trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void axidma_s2mm::s_b_transport(tlm::tlm_generic_payload& trans,
				sc_time& delay)
{
	unsigned char *data = trans.get_data_ptr();
	unsigned int len = trans.get_data_length();
	genattr_extension *genattr;
	unsigned int len_to_copy;
	uint64_t addr;
	bool eop = true;

	trans.get_extension(genattr);
	if (genattr) {
		eop = genattr->get_eop();
	}

	if (regs.sr & AXIDMA_SR_IDLE) {
		/* Put back-pressure.  */
		D(printf("%s: DMA IS IDLE length=%x\n",
				name(), regs.length));
		do {
			wait(ev_dma_copy);
		} while (regs.sr & AXIDMA_SR_IDLE);
	}

	addr = regs.addr_msb;
	addr <<= 32;
	addr += regs.addr;

	len_to_copy = regs.length >= len ? len : regs.length;

	if (use_memcpy) {
		memcpy((void *) addr, data, len_to_copy);
	} else {
		do_dma_trans(tlm::TLM_WRITE_COMMAND, data, addr,
				len_to_copy, delay);
	}

	length_copied += len_to_copy;
	addr += len_to_copy;
	regs.length -= len_to_copy;
	regs.addr_msb = addr >> 32;
	regs.addr = addr;

	if (regs.length == 0 || eop) {
		regs.sr |= AXIDMA_SR_IDLE | AXIDMA_SR_IOC_IRQ;
		regs.length = length_copied;
	}

	ev_update_irqs.notify();
	trans.set_response_status(tlm::TLM_OK_RESPONSE);
}
