/*
 * This VFIO demo application calculates MD5 checksums on files using the
 * pcie-ats-demo.
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

#include <sstream>
#include <iomanip>

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include "systemc"
using namespace sc_core;
using namespace sc_dt;
using namespace std;

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "tlm-modules/tlm-splitter.h"
#include "tlm-bridges/tlm2vfio-bridge.h"

#define SZ_4K (4 * 1024)
#define SZ_32K (32 * 1024)

// Top simulation module.
SC_MODULE(Top)
{
	vfio_dev vdev;

	SC_HAS_PROCESS(Top);

	enum {
		R_CTRL = 0x0,
		R_ADDR_LSB = 0x4,
		R_LENGTH = 0x8,
		R_VAL = 0xC,
		R_STATUS = 0x10,
		R_ADDR_MSB = 0x14,
		R_MD5_RESULT_0 = 0x18,
		R_MD5_RESULT_1 = 0x1C,
		R_MD5_RESULT_2 = 0x20,
		R_MD5_RESULT_3 = 0x24,

		R_CTRL_TRANSLATE = 1 << 0,
		R_CTRL_READ = 1 << 1,
		R_CTRL_WRITE = 1 << 2,
		R_CTRL_MD5SUM = 1 << 3,

		R_STATUS_DONE = 1 << 0,
		R_STATUS_ERR = 1 << 1,
	};

	Top(sc_module_name name, const char *devname, int iommu_group,
			const char *filename) :
		sc_module(name),
		vdev(devname, iommu_group),
		m_filename(filename)
	{
		SC_THREAD(run_md5);
	}

	//
	// Compute the MD5 message digest on input file
	//
	void run_md5()
	{
		uintptr_t map_uint;
		uint64_t map_size;
		uint8_t *tmp;
		struct stat s;
		void *mbuf;
		int fd;

		cout << endl << " * MD5: " << m_filename << endl;

		//
		// Open the file and get file stats
		//
		fd = open(m_filename, O_RDWR);
		if (fd < 0) {
			perror("open");
			exit(EXIT_FAILURE);
		}
		if (fstat(fd, &s) < 0) {
			perror("fstat");
			goto err1;
		}

		//
		// Calculate map_size
		//
		map_size = (s.st_size / SZ_4K) * SZ_4K;
		if (s.st_size % SZ_4K) {
			map_size += SZ_4K;
		}

		//
		// mmap the file
		//
		mbuf = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (mbuf == MAP_FAILED) {
			perror("mmap");
			goto err1;
		}

		//
		// Lock pages to allow direct DMA.
		//
		mlock(mbuf, map_size);

		//
		// VFIO map.
		//
		tmp = (uint8_t *) mbuf;
		map_uint = (uintptr_t) tmp;
		vdev.iommu_map_dma(map_uint, SZ_32K, map_size,
			VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE);

		//
		// MD5 configuration & computation
		//
		write32(R_ADDR_MSB, 0);
		write32(R_ADDR_LSB, SZ_32K);
		write32(R_LENGTH, s.st_size);

		write32(R_STATUS, 0);
		write32(R_CTRL, R_CTRL_MD5SUM);
		wait_for_done();

		//
		// MD5 result
		//
		cout << "   - MD5 result: ";
		for (uint32_t addr = R_MD5_RESULT_0;
			addr <= R_MD5_RESULT_3; addr +=4) {
			uint32_t r = read32(addr);

			cout << hex << right
				<< setw(2) << setfill('0')
				<< ((r >> 0) & 0xFF)
				<< ((r >> 8) & 0xFF)
				<< ((r >> 16) & 0xFF)
				<< ((r >> 24) & 0xFF);
		}
		cout << endl;

		//
		// VFIO unmap.
		//
		vdev.iommu_unmap_dma((uintptr_t) SZ_32K, map_size,
			VFIO_DMA_MAP_FLAG_READ | VFIO_DMA_MAP_FLAG_WRITE);


		//
		// munmap.
		//
		if (munmap(mbuf, s.st_size) ) {
			perror("munmap");
		}

		close(fd);

		sc_stop();
		return;
err1:
		close(fd);
		exit(EXIT_FAILURE);
	}

	void write32(uint32_t addr, uint32_t val)
	{
		// only using BAR0
		uint8_t *map = (uint8_t *) vdev.map[0];

		memcpy_to_io(map + addr, reinterpret_cast<uint8_t*>(&val), sizeof(val));
	}

	uint32_t read32(uint32_t addr)
	{
		// only using BAR0
		uint8_t *map = (uint8_t *) vdev.map[0];
		uint32_t val;

		memcpy_from_io(reinterpret_cast<uint8_t*>(&val),
				map + addr, sizeof(val));

		return val;
	}

	void wait_for_done()
	{
		uint32_t r;

		do {
			r = read32(R_STATUS);
		} while (r == 0);
	}

	const char *m_filename;
};

int sc_main(int argc, char *argv[])
{
	int iommu_group;

	if (argc < 4) {
		printf("%s: device-name iommu-group filename\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	iommu_group = strtoull(argv[2], NULL, 10);
	Top top("Top", argv[1], iommu_group, argv[3]);

	sc_start();

	return 0;
}
