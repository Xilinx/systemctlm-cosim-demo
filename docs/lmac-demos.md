# LMAC Demos

The LMAC demos where developed in the DARPA POSH program in a collaboration
between Xilinx Inc and LeWiz Inc.

To reproduce the demos a large amount of software and RTL need to be built.
This guide is not exhaustive but it will help you through building the main
simulation components and provide pointers for some of the SW components that
are a bit out of scope from this cosim github project.

## QEMU & HW-DTBs

### Build QEMU
You'll have to use Xilinx QEMU that includes the Remote-Port modules for
co-simulation.

```
git clone https://github.com/Xilinx/qemu.git
mkdir build-qemu
cd build-qemu
../qemu/configure --target-list=aarch64-softmmu,microblazeel-softmmu,riscv64-softmmu
make
```

The binaries we'll be using in these examples can be found in the following
paths:
```
build-qemu/riscv64-softmmu/qemu-system-riscv64
build-qemu/aarch64-softmmu/qemu-system-aarch64
```

More details on the build process can be found here:
https://github.com/Xilinx/qemu/blob/master/README.rst

### Build QEMU HW-DTBs

Some of the Xilinx QEMU machine models (e.g ZCU102) are described using device-tree
files. These device-tree's are fed to QEMU so that it can instantiate the emulated
board. We refer to these device-trees as the HW device-trees or HW-DTB.

Here's howto install the HW DTBs into ${HOME}/dts/:
```
git clone https://github.com/Xilinx/qemu-devicetrees.git
cd qemu-devicetrees
make OUTDIR=~/dts/
```

## systemctlm-cosim-demo

This repo contains the SystemC/TLM source code for the co-simulation setup.
It also submodules the LeWiz github repos with the LMAC RTL.

This repo has dependencies to the SystemC libraries and to verilator.
On a modern Ubuntu system, you can do the following:
```
apt-get install libsystemc-dev verilator
```

If your distro doesn't have packages for SystemC and verilator you may
need to build them from source.

Look here for more info:
https://github.com/Xilinx/systemctlm-cosim-demo/blob/master/README

Once the dependencies are build and installed, you can do:

```
git clone https://github.com/Xilinx/systemctlm-cosim-demo.git
cd systemctlm-cosim-demo
git submodule update --init
echo HAVE_VERILOG_VERILATOR=y >>.config.mk
make
```

When done, you should have the following lmac binaries built:

```
riscv_virt_lmac2_demo
riscv_virt_lmac3_demo
zynqmp_lmac2_demo
```

## RISCV64

### RISCV64 SW Stack
To run the RISCV64 demo, you'll need to build a RISCV64 SW stack.
The Linux kernel will have to include the LMAC drivers developed in the posh program.

This guide will guide you through the steps on howto build the RISCV64 SW stack
and howto test it on QEMU:
https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html

When building the Linux kernel, you should use the posh-lmac branch in the following
git repo instead:
https://github.com/edgarigl/linux/tree/posh-lmac

You'll need riscv64 compilers:
```apt-get install gcc-riscv64-linux-gnu```

Start with a default config:
```make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- defconfig```

Then follow-up with enabling what ever you need, including the LeWiz LMAC2 driver (CONFIG_LEWIZ_LMAC2):
```make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- menuconfig```

Build:
```make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu-```

### Add LMAC to virt dtb

When running the QEMU RISCV64 virt machine, QEMU will on-the-fly auto-generate a dtb file
for the Guest Linux. We need to use a modified version of this dtb with the LMAC nodes.

First we'll run QEMU and have it dump the generated dtb file.
We'll need to copy the QEMU command-line from the run examples below
and remove the ```-dtb virt.dtb``` option and append the ``-machine dumpdtb=virt.dtb``
option to make QEMU dump the DTB.

Example:
```
qemu-system-riscv64 -M virt-cosim -smp 4 -serial stdio -display none -m 2G	\
	-bios opensbi-riscv64-virt-fw_jump.bin -kernel Image			\
	-append "root=/dev/vda ro console=ttyS0"				\
	-drive file=rootfs.ext2,format=raw,id=hd0				\
	-device virtio-blk-device,drive=hd0					\
	-object rng-random,filename=/dev/urandom,id=rng0			\
	-device virtio-rng-device,rng=rng0					\
	-device virtio-net-device,netdev=usernet -netdev user,id=usernet	\
	-machine dumpdtb=virt.dtb
```

Now, we'll disassemble it back to source:
```
dtc -I dtb -O dts -o virt.dts virt.dtb
```

And then we'll edit it adding the following at the end, inside of the soc node:
```
                lmac2@0x280030000 {
                        compatible = "lewiz,lmac2";
                        reg = < 0x00 0x28030000 0x00 0x10000 >;
                        mac-address = [ 00 40 8c 00 00 01 ];
                        interrupt-names = "tx\0rx";
                        interrupt-parent = < 0x09 >;
                        interrupts = < 0x32 0x34 >;
                };
```

Convert it back to dtb:
```
dtc -I dts -O dtb -o virt.dtb virt.dts
```

### Running

Once you have a system going it's time to try the co-simulation.
You'll need two terminals, one for QEMU and another for the SystemC simulators.

QEMU:
```
mkdir /tmp/machine-riscv64/
qemu-system-riscv64 -M virt-cosim -smp 4 -serial stdio -display none -m 2G	\
	-dtb virt.dtb -bios opensbi-riscv64-virt-fw_jump.bin -kernel Image	\
	-append "root=/dev/vda ro console=ttyS0"				\
	-drive file=rootfs.ext2,format=raw,id=hd0				\
	-device virtio-blk-device,drive=hd0					\
	-object rng-random,filename=/dev/urandom,id=rng0			\
	-device virtio-rng-device,rng=rng0					\
	-device virtio-net-device,netdev=usernet -netdev user,id=usernet	\
	-netdev user,id=net4							\
	-device remote-port-net,rp-adaptor0=/machine/cosim,rp-chan0=256,rp-chan1=266,netdev=net4 \
	-machine-path /tmp/machine-riscv64
```

SystemC:
```
riscv_virt_lmac2_demo unix:/tmp/machine-riscv64/qemu-rport-_machine_cosim 10000
```

When Linux boots, you should now see the kernel discovering the co-simulated LMAC:
```
lmac2 28030000.lmac2: Lewiz at 0x28030000 mapped to 0xFFFFFFFF44020000, tx-irq=9 rx-irq=11
```
