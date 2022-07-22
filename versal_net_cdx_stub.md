# Launching the versal_net_cdx_stub example

## Overview

Instructions for how to co-simulate Xilinx KSB QEMU together with the
versal_net_cdx_stub example are found below.

## Overview versal_net_cdx_stub

The versal_net_cdx_stub contains a [debug device](debugdev.h) at address
0xe4000000, a [CounterDev](versal_net_cdx_stub.cc#L51) at address 0xe4040000,
2 [Xilinx CDMA devices]
(https://github.com/Xilinx/libsystemctlm-soc/blob/master/soc/dma/xilinx-cdma.h)
at address 0xe4020000 and 0xe4030000, 2 MB memory at address 0xe4100000 and
also 2MB memory at address 0xe4300000. CDMA device 0 uses SMID 0x250 and CDMA
device 1 uses SMID 0x251.


```
#
# Code from versal_net_cdx_stub.cc
#
.
.
.
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
.
.
.
```
### debugdev

Reading the debugdev's register 0 reads out the SystemC simulation time.
Writing to the debugdev's register 0 outputs a debug print in the SystemC
terminal. An example demonstrating how this can be done with the 'devmem'
utility is found below.

### CounterDev

Reading the CounterDev's register 0 reads out the counter value and also
increments the counter after read out. Writing to the CounterDev's register 0
resets the counter to the written value. An example demonstrating how this can
be done with the 'devmem' utility is found below.

### Xilinx CDMA

pg034-axi-cdma.pdf contains more information about how software should interact
with the [Xilinx CDMA devices](https://github.com/Xilinx/libsystemctlm-soc/blob/master/soc/dma/xilinx-cdma.h).

## Download and install SystemC

```
$ tar xzf systemc-2.3.2.tar.gz
$ cd systemc-2.3.2
$ CXXFLAGS=-std=c++11 ./configure
$ make
$ make install
```

## Clone systemctlm-cosim-demo and build the versal_net_cdx_stub

```
$ git clone https://github.com/Xilinx/systemctlm-cosim-demo
$ cd systemctlm-cosim-demo
$ git submodule update --init libsystemctlm-soc
# Replace /path/to/systemc-2.3.2 to where above was installed
$ cat > .config.mk <<EOF
HAVE_VERILOG=n
HAVE_VERILOG_VERILATOR=n
HAVE_VERILOG_VCS=n
SYSTEMC = /path/to/systemc-2.3.2
CXXFLAGS += -std=c++11
EOF
$ make versal_net_cdx_stub
```

## Launch QEMU and the SystemC versal_net_cdx_stub application

In one terminal launch KSB QEMU (use the 'ea' branch of QEMU and the QEMU
device trees).

```
# Create machine directory
$ mkdir /tmp/qemu
# Launch QEMU
$ qemu-system-aarch64  -M arm-generic-fdt  -serial null -serial null -serial stdio -display none -m 2G -kernel /path/to/qtx-images/linux-image -initrd /path/to/rootfs.cpio.gz.u-boot -dtb /path/to/system.dtb -append "rdinit=/sbin/init console=ttyAMA0,115200n8 earlycon=pl011,mmio,0xf1920000,115200n8" -net nic,model=cadence_gem,netdev=net0 -netdev user,id=net0 -net nic,model=cadence_gem,netdev=net1 -netdev user,id=net1 -device virtio-rng-device,bus=virtio_mmio_0.0,rng=rng0 -object rng-random,filename=/dev/urandom,id=rng0  -machine linux=yes -hw-dtb /path/to/dtb-ea/LATEST/SINGLE_ARCH/board-versal-versal-net-psx-cosim-virt.dtb -machine-path /tmp/qemu

```

In a second terminal launch the versal_net_cdx_stub application.

```
$ cd systemctlm-cosim-demo
$ LD_LIBRARY_PATH=/path/to/systemc-2.3.2-cp11/lib-linux64/ ./versal_net_cdx_stub unix:/tmp/qemu/qemu-rport-_amba@0_cosim@0 10000

```

## CounterDev register read and write examples

After linux has boot up one can login into the system with the user 'root' and
password 'root', thereafter one can read/write out the counter register value
using devmem.

```
#
# Read counter value
#
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4040000 32
0x00000000
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4040000 32
0x00000001
#
# Write and reset the counter value to 0x10
#
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4040000 32 0x10
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4040000 32
0x00000010
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4040000 32
0x00000011
```

## debugdev register read and write examples

Reading the debugdev's register 0 reads out the SystemC simulation time.
Writing to the debugdev's register 0 outputs a debug print in the SystemC
terminal.

```
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4000000 32
0xE8BFA8D8
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4000000 32 0xaa
#
# In the SystemC terminal:
#
TRACE:  aa 8597785008 ns diff=67531 us
```

## Read and write to the memory examples

```
# Read the memory
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4100000 32
0x00000000
# Write the memory
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4100000 32 0xaa
root@xilinx-vc-p-a2197-00-reva-x-prc-01-reva-2020_1:~# devmem 0xe4100000 32
0x000000aa
```
