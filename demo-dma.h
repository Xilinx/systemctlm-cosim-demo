/*
 * Copyright (c) 2013 Xilinx Inc.
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

enum {
	DEMODMA_CTRL_RUN  = 1 << 0,
	DEMODMA_CTRL_DONE = 1 << 1,
};

enum {
	DEMODMA_RESP_OKAY               = 0,
	DEMODMA_RESP_BUS_GENERIC_ERROR  = 1,
	DEMODMA_RESP_ADDR_DECODE_ERROR  = 2,
};

class demodma
: public sc_core::sc_module
{
public:
	tlm_utils::simple_initiator_socket<demodma> init_socket;
	tlm_utils::simple_target_socket<demodma> tgt_socket;

	sc_out<bool> irq;
	demodma(sc_core::sc_module_name name);
	SC_HAS_PROCESS(demodma);
private:
	union {
		struct {
			uint32_t dst_addr;
			uint32_t src_addr;
			uint32_t len;
			uint32_t ctrl;
			uint32_t byte_en;
			uint32_t error_resp;
		};
		uint32_t u32[8];
	} regs;

	sc_event ev_dma_copy;
	void do_dma_trans(tlm::tlm_command cmd, unsigned char *buf,
			sc_dt::uint64 addr, sc_dt::uint64 len);
	void do_dma_copy(void);
	void update_irqs(void);

	virtual void b_transport(tlm::tlm_generic_payload& trans,
					sc_time& delay);
};
