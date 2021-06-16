/*
 * The PCIe accelerator model below contains an ATC cache demonstrating how
 * to perform remote port ATS translation requests and also how to receive
 * remote port ATS invalidation requests.
 *
 * Copyright (c) 2021 Xilinx Inc.
 * Written by Francisco Iglesias
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

#ifndef __PCI_ACC_H__
#define __PCI_ACC_H__

#include "tlm.h"
#include "soc/pci/core/pci-device-base.h"
#include "tlm-extensions/atsattr.h"
#include <openssl/md5.h>

#define NR_MMIO_BAR  1
#define NR_IRQ  0

class pcie_acc : public pci_device_base
{
private:
	//
	// Memory regions used by the address translation cache
	//
	class MemoryRegion
	{
	public:
		MemoryRegion(uint64_t virt_addr,
				uint64_t phys_addr,
				uint64_t length,
				uint64_t attributes) :
			m_virt_addr(virt_addr),
			m_phys_addr(phys_addr),
			m_length(length),
			m_attributes(attributes)
		{}

		uint64_t get_virt_addr() { return m_virt_addr; }
		uint64_t get_phys_addr() { return m_phys_addr; }
		uint64_t get_length() { return m_length; }
		uint64_t get_attributes() { return m_attributes; }

		bool contains(uint64_t addr)
		{
			return addr >= m_virt_addr && addr < (m_virt_addr + m_length);
		}

		uint64_t virt_to_phys(uint64_t virt_addr)
		{
			uint64_t m_mask = m_length-1;
			return m_phys_addr | (virt_addr & m_mask);
		}
	private:
		uint64_t m_virt_addr;
		uint64_t m_phys_addr;
		uint64_t m_length;
		uint64_t m_attributes;
	};

	//
	// Address translation cache
	//
	class ATC {
	public:
		tlm_utils::simple_initiator_socket<pci_device_base> &m_ats_req;

		ATC(tlm_utils::simple_initiator_socket<pci_device_base> &ats_req):
			m_ats_req(ats_req)
		{}

		//
		// Transmit ATS translation requests for the region.
		//
		// virt_addr: region start address
		// length: region length
		//
		void do_ats_req(uint64_t virt_addr, uint64_t length)
		{
			//
			// Make sure to have the translations naturally
			// aligned to SZ_4K size
			//
			length += virt_addr & (SZ_4K-1);
			virt_addr &= ~(SZ_4K-1);

                        while (length) {
				atsattr_extension *atsattr = new atsattr_extension();
				sc_time delay(SC_ZERO_TIME);
				tlm::tlm_generic_payload gp;
				uint64_t attr = atsattr_extension::ATTR_WRITE |
						atsattr_extension::ATTR_READ |
						atsattr_extension::ATTR_EXEC;

				gp.set_extension(atsattr);
				gp.set_command(tlm::TLM_IGNORE_COMMAND);

				//
				// Set the ATS translation request's region start
				// address and attributes
				//
				gp.set_address(virt_addr);
				atsattr->set_attributes(attr);

				//
				// Transmit the ATS request
				//
				m_ats_req->b_transport(gp, delay);

				if (gp.get_response_status() == tlm::TLM_OK_RESPONSE &&
					atsattr->get_result() == atsattr_extension::RESULT_OK) {

					//
					// Make sure to have the address
					// aligned to the returned length
					//
					virt_addr &= ~(atsattr->get_length()-1);

					//
					// Translation succeded, add into the ATC cache
					//
					m_regions.push_back(MemoryRegion(virt_addr, gp.get_address(),
									atsattr->get_length(),
									atsattr->get_attributes()));

					if (atsattr->get_length() > length) {
						//
						// Last translations has been received
						//
						break;
					}

					length -= atsattr->get_length();
					virt_addr += atsattr->get_length();
				}
			}
		}

		//
		// Check if the ATC contains a translation for a virtual address.
		//
		// virt_addr: the virtual address to perform the lookup for
		//
		// returns: true if the cache contains a translation else false
		//
		bool contains(uint64_t virt_addr)
		{
			std::vector<MemoryRegion>::iterator it;

			for (it = m_regions.begin(); it != m_regions.end(); it++) {
				MemoryRegion &r = (*it);

				if (r.contains(virt_addr)) {
					return true;
				}
			}
			return false;
		}

		//
		// Translate a virtual address into its physical address.
		//
		// virt_addr: the address to perform the translation for
		//
		// returns: the physical address
		//
		uint64_t virt_to_phys(uint64_t virt_addr)
		{
			std::vector<MemoryRegion>::iterator it;

			for (it = m_regions.begin(); it != m_regions.end(); it++) {
				MemoryRegion &r = (*it);

				if (r.contains(virt_addr)) {
					return r.virt_to_phys(virt_addr);
				}
			}
			return 0;
		}

		//
		// Test that attributes on an address are allowed.
		//
		// virt_addr: the address to test the attributes on
		// attr: the attributes to verify if they are allowed
		//
		// returns: true if the attributes are ok
		//
		bool test_attr(uint64_t virt_addr, uint64_t attr)
		{
			std::vector<MemoryRegion>::iterator it;

			for (it = m_regions.begin(); it != m_regions.end(); it++) {
				MemoryRegion &r = (*it);

				if (r.contains(virt_addr)) {
					return r.get_attributes() & attr;
				}
			}
			return false;
		}

		//
		// Invalidate a range in the ATC cache.
		//
		// virt_addr: the start address of the range
		// length: the length of the range
		//
		void invalidate(uint64_t virt_addr, uint64_t length)
		{

			std::vector<MemoryRegion>::iterator it;

			for (it = m_regions.begin(); it != m_regions.end();) {
				MemoryRegion &r = (*it);
				uint64_t region_start = r.get_virt_addr();
				uint64_t region_end = region_start +
							r.get_length() - 1;

				if (in_range(region_start, virt_addr, length) ||
					in_range(region_end, virt_addr, length)) {
					m_regions.erase(it);
				} else {
					it++;
				}
			}
		}

	private:
		bool in_range(uint64_t addr, uint64_t start, uint64_t len)
		{
			return addr >= start && addr < (start + len);
		}

		std::vector<MemoryRegion> m_regions;
	};

	enum {
		//
		// Register addresses
		//
		R_CTRL = 0x0,
		R_ADDR = 0x4,
		R_LENGTH = 0x8,
		R_VAL = 0xC,
		R_STATUS = 0x10,
		R_MSB_ADDR = 0x14,
		R_MD5_RESULT_0 = 0x18,
		R_MD5_RESULT_1 = 0x1C,
		R_MD5_RESULT_2 = 0x20,
		R_MD5_RESULT_3 = 0x24,

		//
		// R_CTRL bits
		//
		R_CTRL_TRANSLATE = 1 << 0,
		R_CTRL_READ = 1 << 1,
		R_CTRL_WRITE = 1 << 2,
		R_CTRL_MD5SUM = 1 << 3,

		//
		// R_STATUS bits
		//
		R_STATUS_DONE = 1 << 0,
		R_STATUS_ERR = 1 << 1,
	};

	//
	// Configuration messages enter here
	//
	void config_b_transport(tlm::tlm_generic_payload &trans, sc_time &delay) {
		trans.set_response_status(tlm::TLM_OK_RESPONSE);
	}

	void reset(void) {
	}

	//
	// BAR0 read and writes enter here
	//
	void bar_b_transport(int bar_nr, tlm::tlm_generic_payload &trans, sc_time &delay) {
		tlm::tlm_command cmd = trans.get_command();
		sc_dt::uint64 addr = trans.get_address();
		unsigned char *data = trans.get_data_ptr();
		unsigned int len = trans.get_data_length();
		unsigned char *byte_en = trans.get_byte_enable_ptr();
		unsigned int s_width = trans.get_streaming_width();
		uint32_t v = 0;

		if (byte_en || len > 4 || s_width < len) {
			goto err;
		}

		if (cmd == tlm::TLM_READ_COMMAND) {

			switch (addr) {
			case R_CTRL:
				v = 0;
				break;
			case R_ADDR:
				v = regs.addr_lsb;
				break;
			case R_LENGTH:
				v = regs.length;
				break;
			case R_VAL:
				v = regs.value;
				break;
			case R_STATUS:
				v = regs.status;
				break;
			case R_MSB_ADDR:
				v = regs.addr_msb;
				break;
			case R_MD5_RESULT_0:
				v = regs.md5_result0;
				break;
			case R_MD5_RESULT_1:
				v = regs.md5_result1;
				break;
			case R_MD5_RESULT_2:
				v = regs.md5_result2;
				break;
			case R_MD5_RESULT_3:
				v = regs.md5_result3;
				break;
			default:
				break;
			}

			memcpy(data, &v, len);
		} else if (cmd == tlm::TLM_WRITE_COMMAND) {

			memcpy(&v, data, len);

			switch (addr) {
			case R_CTRL:
				if (v & R_CTRL_TRANSLATE) {
					m_ats_req_event.notify();
				} else if (v & R_CTRL_READ) {
					m_read_event.notify();
				} else if (v & R_CTRL_WRITE) {
					m_write_event.notify();
				} else if (v & R_CTRL_MD5SUM) {
					m_md5_event.notify();
				}
				break;
			case R_ADDR:
				regs.addr_lsb = v;
				break;
			case R_LENGTH:
				regs.length = v;
				break;
			case R_VAL:
				regs.value = v;
				break;
			case R_STATUS:
				regs.status = v;
				break;
			case R_MSB_ADDR:
				regs.addr_msb = v;
				break;
			default:
				break;
			}
		} else {
			goto err;
		}

		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		return;
err:
		trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
		return;
	}

	//
	// Remote port ATS invalidation messages are received here.
	//
	void b_transport_ats_inv(tlm::tlm_generic_payload &trans,
				sc_time &delay)
	{
		atsattr_extension *atsattr;

		trans.get_extension(atsattr);
		if (atsattr) {
			//
			// Request ATC invalidation
			//
			m_atc.invalidate(trans.get_address(), atsattr->get_length());
		}

		trans.set_response_status(tlm::TLM_OK_RESPONSE);
	}

	//
	// Read len bytes from an already translated (physical) address and place
	// into provided data buffer.
	//
	// phys_addr: The physical address to read from
	// data: The data buffer where the read data will be placed
	// len: The amount of data to read
	//
	void phys_read(uint64_t phys_addr, uint8_t *data, unsigned long len)
	{
		atsattr_extension *atsattr = new atsattr_extension();
		sc_time delay(SC_ZERO_TIME);
		tlm::tlm_generic_payload gp;

		gp.set_extension(atsattr);
		gp.set_command(tlm::TLM_READ_COMMAND);
		gp.set_address(phys_addr);
		gp.set_data_ptr(data);
		gp.set_data_length(len);
		gp.set_streaming_width(len);

		atsattr->set_attributes(atsattr_extension::ATTR_PHYS_ADDR);

		dma->b_transport(gp, delay);

		assert(gp.get_response_status() == tlm::TLM_OK_RESPONSE);
	}


	//
	// Read 4 bytes from an already translated (physical) address and place
	// into regs.value.
	//
	// phys_addr: The physical address to read from
	//
	void phys_read32(uint64_t phys_addr)
	{
		uint32_t data;

		phys_read(phys_addr, reinterpret_cast<uint8_t*>(&data),
				sizeof(data));

		regs.value = data;
	}

	//
	// Write the 4 bytes in regs.value to an already translated (physical)
	// address.
	//
	// phys_addr: The physical address to write to
	//
	void phys_write32(uint64_t phys_addr)
	{
		atsattr_extension *atsattr = new atsattr_extension();
		sc_time delay(SC_ZERO_TIME);
		tlm::tlm_generic_payload gp;
		uint32_t data = regs.value;
		uint8_t *d = reinterpret_cast<uint8_t*>(&data);

		gp.set_extension(atsattr);
		gp.set_command(tlm::TLM_WRITE_COMMAND);
		gp.set_address(phys_addr);
		gp.set_data_ptr(d);
		gp.set_data_length(4);
		gp.set_streaming_width(4);

		atsattr->set_attributes(atsattr_extension::ATTR_PHYS_ADDR);

		dma->b_transport(gp, delay);

		assert(gp.get_response_status() == tlm::TLM_OK_RESPONSE);
	}

	enum { SZ_4K = 4096 };

	//
	// This thread waits for an 'm_ats_req_event' which is notified when
	// the R_CTRL register is written with the value R_CTRL_TRANSLATE.
	// After receiving the event the thread will let the ATC perform ATS
	// translation requests for the address range starting at address in
	// the registers R_ADDR_MSB and R_ADDR_LSB and with the length in
	// programmed into R_LENGTH. The translation is done in SZ_4K sizes.
	//
	// After the translation has completed R_STATUS_DONE is set in the
	// R_STATUS register.
	//
	void ats_req_thread()
	{
		while (true) {
			uint64_t addr;
			uint64_t length;

			wait(m_ats_req_event);

			addr = static_cast<uint64_t>(regs.addr_msb) << 32 |
				regs.addr_lsb;

			length = regs.length;

			for (; length > 0; length -= SZ_4K, addr += SZ_4K) {
				uint64_t len = length;

				if (!m_atc.contains(addr)) {
					m_atc.do_ats_req(addr, SZ_4K);
				}

				if (len < SZ_4K) {
					// Done
					break;
				}
			}

			regs.status = R_STATUS_DONE;
		}
	}

	//
	// This thread waits for an 'm_read_event' which is notified when
	// the R_CTRL register is written with the value R_CTRL_READ.
	// After receiving the event the thread will let the ATC translate the
	// the virtual address in the registers R_ADDR_MSB and R_ADDR_LSB (if
	// the ATC does not contain the address an ATS translation request
	// will be initiated). After obtaining the physical address the thread
	// reads 4 bytes from the address and places it in the R_VAL register.
	//
	// After the read has completed R_STATUS_DONE is set in the R_STATUS
	// register.
	//
	void read_thread()
	{
		while (true) {
			uint64_t virt_addr;

			wait(m_read_event);

			virt_addr = static_cast<uint64_t>(regs.addr_msb) << 32 |
					regs.addr_lsb;

			if (!m_atc.contains(virt_addr)) {
				m_atc.do_ats_req(virt_addr, SZ_4K);
			}

			if (m_atc.test_attr(virt_addr,
					atsattr_extension::ATTR_READ)) {
				uint64_t phys_addr =
					m_atc.virt_to_phys(virt_addr);

				phys_read32(phys_addr);
			}

			regs.status = R_STATUS_DONE;
		}
	}

	//
	// This thread waits for an 'm_write_event' which is notified when
	// the R_CTRL register is written with the value R_CTRL_WRITE.
	// After receiving the event the thread will let the ATC translate the
	// the virtual address in the registers R_ADDR_MSB and R_ADDR_LSB (if
	// the ATC does not contain the address an ATS translation request
	// will be initiated). After obtaining the physical address the thread
	// writes the 4 bytes in the R_VAL register to the address.
	//
	// After the write has completed R_STATUS_DONE is set in the R_STATUS
	// register.
	//
	void write_thread()
	{
		while (true) {
			uint64_t virt_addr;

			wait(m_write_event);

			virt_addr = static_cast<uint64_t>(regs.addr_msb) << 32 |
					regs.addr_lsb;

			if (!m_atc.contains(virt_addr)) {
				m_atc.do_ats_req(virt_addr, SZ_4K);
			}

			if (m_atc.test_attr(virt_addr,
					atsattr_extension::ATTR_WRITE)) {
				uint64_t phys_addr =
					m_atc.virt_to_phys(virt_addr);

				phys_write32(phys_addr);
			}

			regs.status = R_STATUS_DONE;
		}
	}

	//
	// This thread waits for an 'm_md5_event' which is notified when
	// the R_CTRL register is written with the value R_CTRL_MD5SUM.
	// After receiving the event the thread will compute the MD5 message
	// digest of the virtual address area starting from the address
	// configured in the address registers R_ADDR_MSB and R_ADDR_LSB
	// and with length configured in the R_LENGTH register. The
	// result is placed to be read out in the R_MD5_RESULT_X registers.
	//
	// After the MD5 computation has completed R_STATUS_DONE is set in the
	// R_STATUS register. In case of an error R_STATUS_ERR is notified.
	//
	void md5_thread()
	{
		while (true) {
			unsigned char res[MD5_DIGEST_LENGTH];
			uint8_t data[SZ_4K];
			uint64_t virt_addr;
			unsigned long len;
			MD5_CTX ctx;

			wait(m_md5_event);

			virt_addr = static_cast<uint64_t>(regs.addr_msb) << 32 |
					regs.addr_lsb;
			len = regs.length;

			if (MD5_Init(&ctx) == 0) {
				regs.status = R_STATUS_ERR | R_STATUS_DONE;
				continue;
			}

			while (len) {
				unsigned long md5_len;
				uint64_t phys_addr;
				uint64_t mask = (SZ_4K - 1);

				if (!m_atc.contains(virt_addr)) {
					m_atc.do_ats_req(virt_addr, len);
				}

				if (!m_atc.test_attr(virt_addr,
						atsattr_extension::ATTR_READ)) {
					// Error
					break;
				}

				phys_addr = m_atc.virt_to_phys(virt_addr);

				//
				// Adjust length to not cross SZ_4K boundaries
				//
				md5_len = (len <= SZ_4K) ? len : SZ_4K;
				md5_len -= (phys_addr & mask);

				phys_read(phys_addr, data, md5_len);

				MD5_Update(&ctx, reinterpret_cast<void*>(data), md5_len);

				virt_addr += md5_len;
				len -= md5_len;
			}

			if (len != 0) {
				regs.status = R_STATUS_ERR | R_STATUS_DONE;
				continue;
			}

			MD5_Final(res, &ctx);

			regs.md5_result0 = (res[0] << 0) | (res[1] << 8) |
						(res[2] << 16) | (res[3] << 24);
			regs.md5_result1 = (res[4] << 0) | (res[5] << 8) |
						(res[6] << 16) | (res[7] << 24);
			regs.md5_result2 = (res[8] << 0) | (res[9] << 8) |
						(res[10] << 16) | (res[11] << 24);
			regs.md5_result3 = (res[12] << 0) | (res[13] << 8) |
						(res[14] << 16) | (res[15] << 24);

			regs.status = R_STATUS_DONE;
		}
	}

	//
	// events
	//
	sc_event m_ats_req_event;
	sc_event m_read_event;
	sc_event m_write_event;
	sc_event m_md5_event;

	//
	// Address translation cache
	//
	ATC m_atc;

	//
	// Registers
	//
	union {
		struct {
			uint32_t ctrl;
			uint32_t addr_lsb;
			uint32_t length;
			uint32_t value;
			uint32_t status;
			uint32_t addr_msb;

			uint32_t md5_result0;
			uint32_t md5_result1;
			uint32_t md5_result2;
			uint32_t md5_result3;
		};
		uint32_t u32[10];
	} regs;
public:
	SC_HAS_PROCESS(pcie_acc);
	sc_in<bool> rst;

	pcie_acc(sc_core::sc_module_name name) :
		pci_device_base(name, NR_MMIO_BAR, NR_IRQ),
		m_ats_req_event("ats-req-event"),
		m_read_event("read-event"),
		m_write_event("write-event"),
		m_md5_event("md5-event"),
		m_atc(ats_req),
		rst("rst")
	{
		memset(&regs, 0, sizeof regs);

		SC_METHOD(reset);
		dont_initialize();
		sensitive << rst;

		SC_THREAD(ats_req_thread);
		SC_THREAD(read_thread);
		SC_THREAD(write_thread);
		SC_THREAD(md5_thread);
	}
};

#endif /* __PCI_ACC_H__ */
