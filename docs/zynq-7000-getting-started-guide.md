# Setting up and Running a Co-Simulation with a System Clock Example

This demonstration shows how to compile and run the Co-Simulation demo of Buildroot in QEMU with a simulated device in SystemC.  **Buildroot was selected for this demo, but other Linux Kernel's can be built and deployed in QEMU and used as well.** This configuration is tested working for Ubuntu 18.0.4 and assumes that a `cosim` directory is created in your home directory. This walkthrough also assumes that the device being emulated by QEMU is the Xilinx Zynq-7000 SoC.  This SoC seemed like a good candidate but the concept can apply to any QEMU machine which plugs in a compatible remoteport bus interface.

## Dependencies

Below are the dependencies needed to compile all the libraries in this demo:

```bash
sudo apt update
sudo apt install cmake gmake gcc qemu-kvm qemu-system qemu-user-static verilator
```

## Setup and Compilation

Run these commands to clone and build the necessary repos (`~/cosim` assumed as the base directory).

### Create the base directory

```bash
mkdir ~/cosim
```

### SystemC Setup

```bash
# Build and install the binaries
cd ~/cosim
SYSC_VERSION=systemc-2.3.2
wget https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.2.tar.gz
tar xf systemc.2.3.2.tar.gz
cd systemc-2.3.2/
CXXFLAGS=-std=c++11 ./configure ; make
sudo make install
```

### QEMU Setup

```bash
cd ~/cosim
git clone https://github.com/Xilinx/qemu.git
cd qemu
git checkout 7e3e3ae09af8abe70383a5
git submodule update --init dtc

# Configure and build
./configure --target-list="arm-softmmu,aarch64-softmmu,microblazeel-softmmu" --enable-fdt --disable-kvm --disable-xen
make -j$((`nproc`+1))
sudo make install
export SYSTEM=/cosim/systemc-2.3.2/
```

### Demo Setup

```bash
# Clone the repo and the submodules
cd ~/cosim
git clone https://github.com/Xilinx/systemctlm-cosim-demo.git
cd systemctlm-cosim-demo
sed -i 's~../libsystemctlm-soc.git~https://github.com/Xilinx/libsystemctlm-soc.git~g' .gitmodules
git submodule sync --recursive && git submodule update --init libsystemctlm-soc

# Create the Makefile configeration
cat << EOF | tee .config.mk
CXXFLAGS=-std=c++11
SYSTEMC=${HOME/cosim}
HAVE_VERILOG=n
HAVE_VERILOG_VERILATOR=n
HAVE_VERILOG_VCS=n
VM_TRACE=1
EOF*

# Build the demo
make zynq_demo
```

### Buildroot Setup

```bash
cd ~/cosim
git clone https://github.com/buildroot/buildroot.git
cd buildroot
git checkout 36edacce9c2c3b90f9bb11
mkdir handles
```

For this walkthough we will be configuring the demo to emulate the Zynq-7000 SoC. 

```bash
# Pull the .dtsi files for the Zynq-7000
wget https://raw.githubusercontent.com/Xilinx/qemu-devicetrees/master/zynq-pl-remoteport.dtsi
wget https://raw.githubusercontent.com/Xilinx/linux-xlnx/master/arch/arm/boot/dts/zynq-7000.dtsi
wget https://raw.githubusercontent.com/Xilinx/device-tree-xlnx/master/device_tree/data/kernel_dtsi/2017.3/BOARD/zc702.dtsi

# Setup the Buildroot compilation configeration
rm .config .config.old  -f
cat << EOF | tee ".config"
BR2_arm=y
BR2_cortex_a9=y
BR2_ARM_ENABLE_NEON=y
BR2_ARM_ENABLE_VFP=y
BR2_TOOLCHAIN_EXTERNAL=y
BR2_TOOLCHAIN_EXTERNAL_BOOTLIN=y
BR2_TOOLCHAIN_EXTERNAL_BOOTLIN_ARMV7_EABIHF_GLIBC_STABLE=y
BR2_TARGET_GENERIC_GETTY_PORT="ttyPS0"
BR2_LINUX_KERNEL=y
BR2_LINUX_KERNEL_CUSTOM_VERSION=y
BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="5.4.75"
BR2_LINUX_KERNEL_USE_ARCH_DEFAULT_CONFIG=y
BR2_LINUX_KERNEL_UIMAGE=y
BR2_LINUX_KERNEL_UIMAGE_LOADADDR="0x8000"
BR2_LINUX_KERNEL_DTS_SUPPORT=y
BR2_LINUX_KERNEL_CUSTOM_DTS_PATH="zynq-7000.dtsi  zynq-zc702.dts zynq-pl-remoteport.dtsi"
BR2_TARGET_ROOTFS_CPIO=y
BR2_TARGET_ROOTFS_CPIO_GZIP=y
BR2_TARGET_ROOTFS_EXT2=y
#BR2_TARGET_ROOTFS_TAR is not set
EOF

# Build
make clean && make olddefconfig
./utils/br/make
```

## Running the Demo

In order to run the co-simulation demo, you will need two shells open. One for QEMU and the other for SystemC. In the first shell navigate to the `systemctlm-cosim-demo` directory (`cd ~/cosim/systemctlm-cosim-demo`). This command will start the SystemC simulation:

```bash
LD_LIBRARY_PATH=${SYSTEMC}/systemc-2.3.2/src/.libs/ ./zynq_demo 	unix:${HOME}/cosim/buildroot/handles/qemu-rport-_cosim@0 1000000
```

Start the QEMU instance in the second shell by running (in any directory):

```bash
${SYSTEMC}/qemu/aarch64-softmmu/qemu-system-aarch64 -M arm-generic-fdt-7series -m 1G -kernel ${SYSTEMC}/buildroot/output/images/uImage -dtb ${SYSTEMC}/buildroot/output/images/zynq-zc702.dtb --initrd ${SYSTEMC}/buildroot/output/images/rootfs.cpio.gz -serial /dev/null -serial mon:stdio -display none -net nic -net nic -net user -machine-path ${SYSTEMC}/buildroot/handles -icount 0,sleep=off -rtc clock=vm -sync-quantum 1000000
```

## Example Output

Below is some example output showing the Buildroot instance querying the SystemC simulated environment. **This demo shows how the system clock changes after each read from the memory address. The clock's memory address is 0x40000000. The value returned is the value of the system clock at that period of time**.:

```bash
Welcome to Buildroot
buildroot login: root
# devmem 0x40000000
0x87AB9927
#devmem 0x400000000
0x8BFF63F0
# devmem 0x40000000
0x8F1F7EC3
# devmem 0x40000000
0x917A5800
# uname -a
Linux buildroot 5.4.75-g7441ac1188d7-dirty #3 SMP Tue Apr 13 08:00:50 CST 2021 armv71 GNU/Linux
#cat /proc/cpuinfo
processor       : 0
model name      : ARMv7 Processor rev 0 (v71)
BogoMIPS        : 2164.32
Features        : half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpd32
CPU inplementer : 0x41
CPU architecture: 7
CPU variant     :0x0
CPU part        :0xc09
CPU revision    : 0

processor       : 1
model name      : ARMv7 Processor rev 0 (v71)
BogoMIPS        : 2118.45
Features        :half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpd32
CPU implementer : 0x41
CPU architecture: 7
CPU variant     : 0x0
CPU part        : 0xc09
CPU revision    : 0

Hardware        : Xilinx Zynq Platform
Revision        : 0000
Serial          : 0000000000000000
```



