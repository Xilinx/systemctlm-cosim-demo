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
	AXIDMA_CR_RS		= 1 << 0,
	AXIDMA_CR_RESET		= 1 << 2,
	AXIDMA_CR_KEYHOLE	= 1 << 3,
	AXIDMA_CR_CYCLIC_BD	= 1 << 4,
	AXIDMA_CR_IOC_IRQ_EN	= 1 << 12,
};

enum {
	AXIDMA_SR_HALTED	= 1 << 0,
	AXIDMA_SR_IDLE		= 1 << 1,
	AXIDMA_SR_SGINCLD	= 1 << 3,
	AXIDMA_SR_IOC_IRQ	= 1 << 12,
};

enum {
	AXIDMA_R_CR		= 0x00 / 4,
	AXIDMA_R_SR		= 0x04 / 4,
	AXIDMA_R_ADDR		= 0x18 / 4,
	AXIDMA_R_ADDR_MSB	= 0x1c / 4,
	AXIDMA_R_LENGTH		= 0x28 / 4,
	AXIDMA_R_MAX		= 0x2c / 4,
};

/* Base class common to both the mm2s and s2mm channels.  */
class axidma
: public sc_core::sc_module
{
public:
	tlm_utils::simple_initiator_socket<axidma> init_socket;
	tlm_utils::simple_target_socket<axidma> tgt_socket;

	sc_out<bool> irq;
	axidma(sc_core::sc_module_name name, bool use_memcpy = false);
	SC_HAS_PROCESS(axidma);
protected:
	union {
		struct {
			uint32_t cr;
			uint32_t sr;
			uint32_t rsv0[AXIDMA_R_ADDR - AXIDMA_R_SR - 1];
			uint32_t addr;
			uint32_t addr_msb;
			uint32_t rsv1[AXIDMA_R_LENGTH - AXIDMA_R_ADDR_MSB - 1];
			uint32_t length;
		};
		uint32_t u32[AXIDMA_R_MAX];
	} regs;
	// S2M needs to keep track of the number of bytes actually copied.
	uint32_t length_copied;

	bool use_memcpy;

	sc_event ev_update_irqs;
	sc_event ev_dma_copy;
	virtual void do_dma_copy(void) {};
	void do_dma_trans(tlm::tlm_command cmd, unsigned char *buf,
			sc_dt::uint64 addr, sc_dt::uint64 len, sc_time &delay);
	void update_irqs(void);

private:
	virtual void b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
};

class axidma_mm2s : public axidma
{
public:
	tlm_utils::simple_initiator_socket<axidma_mm2s> stream_socket;
	axidma_mm2s(sc_core::sc_module_name name, bool use_memcpy = false);
protected:
	virtual void do_dma_copy(void);
private:
	void do_stream_trans(tlm::tlm_command cmd, unsigned char *buf,
			sc_dt::uint64 addr, sc_dt::uint64 len, bool eop, sc_time &delay);
};

class axidma_s2mm : public axidma
{
public:
	tlm_utils::simple_target_socket<axidma_s2mm> stream_socket;
	axidma_s2mm(sc_core::sc_module_name name, bool use_memcpy = false);
protected:
	virtual void do_dma_copy(void);
private:
	void s_b_transport(tlm::tlm_generic_payload& trans, sc_time& delay);
};
