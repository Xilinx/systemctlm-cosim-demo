# Running the cpm5-qdma-demo / cpm4-qdma-demo

## Overview

These demos show an x86 host co-simulating with an attached PCIe endpoint
containing a CPM5 QDMA subsystem model (cpm5-qdma-demo) or a CPM4 QDMA
subsystem model (cpm4-qdma-demo). The x86 host is emulated by QEMU and the
PCIe End-point is modelled in SystemC.

Instructions for how to run an Ubuntu based guest system with Xilinx QEMU
together with the cpm5-qdma-demo or cpm4-qdma-demo are found below. The
instructions also demonstrate how to exercise the CPM5 (or CPM4) QDMA
model with Xilinx open source QDMA driver and userspace applications from
inside the Ubuntu guest system.

## Build SystemC 2.3.3 and Xilinx QEMU

Follow the instructions on how to build Acelleras SystemC library and Xilinx
QEMU found in the
[zynq-7000-getting-started-guide](../../docs/zynq-7000-getting-started-guide.md#systemc-setup).

## Build the cpm5-qdma-demo and cpm4-qdma-demo

Make sure to have installed required systemctlm-cosim-demo dependencies listed
in the project [README](../../README).

```
$ sudo apt-get install linux-libc-dev
```

Download and build the cpm5-qdma-demo and cpm4-qdma-demo by following
the following instructions.

```
$ git clone https://github.com/Xilinx/systemctlm-cosim-demo.git
$ cd systemctlm-cosim-demo
$ git submodule update --init libsystemctlm-soc
$ git submodule update --init pcie-model
# Change /path/to to the location where the SystemC library was installed
$ cat <<EOF > .config.mk
SYSTEMC = /path/to/systemc-2.3.3/
EOF
$ make pcie/versal/cpm5-qdma-demo pcie/versal/cpm4-qdma-demo
```

## Preparing the Ubuntu cloud image VM

Download the the Ubuntu cloud image.

```
~$ cd ~/Downloads/
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/ubuntu-20.04-server-cloudimg-amd64.img
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/unpacked/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/unpacked/ubuntu-20.04-server-cloudimg-amd64-initrd-generic
```

Resize the image to 30G.

```
~$ qemu-img resize ~/Downloads/ubuntu-20.04-server-cloudimg-amd64.img 30G
```

Create a disk image with user-data to be used for starting the cloud
image.

```
~$ sudo apt-get install cloud-image-utils
~$ cd ~/Downloads
~$ cat >user-data <<EOF
#cloud-config
password: pass
chpasswd: { expire: False }
ssh_pwauth: True
EOF
~$ cloud-localds user-data.img user-data
```

## Launch the cloud image

Launch the Ubuntu cloud image in QEMU using the following command (please
change 'accel=kvm' to 'accel=tcg' and remove '-enable-kvm' in case the user is
not allowed to use kvm):

```
#
# Change '/path/to's to the location where the Xilinx QEMU binary was
# installed and to the locations where the different images are found.
#
$ /path/to/qemu-system-x86_64                                                         \
    -M q35,accel=kvm,kernel-irqchip=split -cpu qemu64,rdtscp                          \
    -m 4G -smp 4 -enable-kvm -display none                                            \
    -kernel /path/to/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic               \
    -append "root=/dev/sda1 rootwait console=tty1 console=ttyS0 intel_iommu=on"       \
    -initrd /path/to/ubuntu-20.04-server-cloudimg-amd64-initrd-generic                \
    -serial mon:stdio -device intel-iommu,intremap=on,device-iotlb=on                 \
    -drive file=/path/to/ubuntu-20.04-server-cloudimg-amd64.img                       \
    -drive file=/path/to/user-data.img,format=raw                                     \
    -device ioh3420,id=rootport1,slot=1                                               \
    -device remote-port-pci-adaptor,bus=rootport1,id=rp0                              \
    -machine-path /tmp/machine-x86/                                                   \
    -device virtio-net-pci,netdev=net0 -netdev type=user,id=net0                      \
    -device remote-port-pcie-root-port,id=rprootport,slot=0,rp-adaptor0=rp,rp-chan0=0
```
Above command provides network access through a virtio-net-pci device. Once the
system has booted login with the user 'ubuntu' and password 'pass'.

## Launch the cpm5-qdma-demo or the cpm4-qdma-demo

Run the following commands in a second terminal in case the cpm5-qdma-demo is
the desired demo to launch:

```
# Change /path/to to the location where the SystemC library was installed.
$ cd systemctlm-cosim-demo
$ LD_LIBRARY_PATH=/path/to/systemc-2.3.3/lib-linux64/ ./pcie/versal/cpm5-qdma-demo unix:/tmp/machine-x86/qemu-rport-_machine_peripheral_rp0_rp 10000
```

In case it instead is the cpm4-qdma-demo that is the desired demo to launch one
can issue below commands in the second terminal.

```
# Change /path/to to the location where the SystemC library was installed.
$ cd systemctlm-cosim-demo
$ LD_LIBRARY_PATH=/path/to/systemc-2.3.3/lib-linux64/ ./pcie/versal/cpm4-qdma-demo unix:/tmp/machine-x86/qemu-rport-_machine_peripheral_rp0_rp 10000
```

## Build the QDMA driver in the Ubuntu cloud image

Login into the Ubuntu cloud image and install below packages:
```
$ sudo apt update
$ sudo apt install make gcc libaio1 libaio-dev kmod
```

Download and compile Xilinx opensource QDMA driver and userspace applications:
```
$ mkdir ~/Downloads
$ cd ~/Downloads
$ git clone https://github.com/Xilinx/dma_ip_drivers.git
$ cd ~/Downloads/dma_ip_drivers/QDMA/linux-kernel
$ make TANDEM_BOOT_SUPPORTED=1
$ ls ~/Downloads/dma_ip_drivers/QDMA/linux-kernel/bin
dma-ctl          dma-latency  dma-to-device  qdma-pf.ko
dma-from-device  dma-perf     dma-xfer       qdma-vf.ko
```

## Exercise the CPM5/CPM4 QDMA model and transfer data to the SBI keyhole aperture

The following commands demonstrate how to exercise the CPM5 (or CPM4) QDMA
model and start a transfer of a fake PDI file residing in the x86 host to the
SBI keyhole aperture address (the same commands can be used with both the
cpm5-qdma-demo and the cpm4-qdma-demo).

Become the root user in the Ubuntu cloud image and load the QDMA driver:
```
$ cd ~/Downloads/dma_ip_drivers/QDMA/linux-kernel
# Switch to the root user
$ sudo su
$ insmod bin/qdma-pf.ko
```

The following should appear in the dmesg:
```
[  494.366781] qdma_pf:qdma_mod_init: Xilinx QDMA PF Reference Driver v2022.1.3.3.
[  494.371535] qdma_pf:probe_one: 0000:02:00.0: func 0x0, p/v 0/0,0x0000000000000000.
[  494.371698] qdma_pf:probe_one: Configuring '02:00:0' as master pf
[  494.371714] qdma_pf:probe_one: Driver is loaded in auto(0) mode
[  494.371836] qdma_pf:qdma_device_open: qdma-pf, 02:00.00, pdev 0x00000000dbb46fb0, 0x10ee:0x903f.
[  494.380509] Device Type: Versal Hard CPM5
[  494.380535] IP Type: Versal Hard IP
[  494.380551] Vivado Release: vivado 2022.1
[  494.381092] qdma_pf:qdma_device_attributes_get: qdma02000-p0000:02:00.0: num_pfs:1, num_qs:2, flr_present:0, st_en:1, mm_en:1, mm_cmpt_en:0, mailbox_en:0, mm_channel_max:2, qid2vec_ctx:0, cmpt_ovf_chk_dis:1, mailbox_intr:1, sw_desc_64b:1, cmpt_desc_64b:1, dynamic_bar:1, legacy_intr:1, cmpt_trig_count_timer:1
[  494.381344] qdma_pf:qdma_device_open: Vivado version = vivado 2022.1
[  494.381628] qdma_dev_entry_create: Created the dev entry successfully
[  494.382041] qdma_pf:intr_setup: current device supports only (8) msix vectors per function. ignoring input for (32) vectors
[  494.414062] qdma_pf:xdev_identify_bars: AXI Master Lite BAR 2.
[  494.414123] qdma_pf:xdev_identify_bars: AXI Bridge Master BAR 4.
[  494.414167] qdma_pf:qdma_device_open: 0000:02:00.0, 02000, pdev 0x00000000dbb46fb0, xdev 0x00000000d4226954, ch 2, q 0, vf 0.
```

Configure and start a QDMA driver queue with below commands:
```
# Set the maximum number of queue for the QDMA.
$ echo 1 > /sys/bus/pci/devices/0000:02:00.0/qdma/qmax

# Configure the first queue: Memory Mapped, Host to Card.
$ ./bin/dma-ctl qdma02000 q add idx 0 mode mm dir h2c
qdma02000-MM-0 H2C added.
Added 1 Queues.

# Start that queue.
$ ./bin/dma-ctl qdma02000 q start idx 0 dir h2c aperture_sz 4096
dma-ctl: Info: Default ring size set to 2048
1 Queues started, idx 0 ~ 0.
```

Exercise the CPM5 (or CPM4) QDMA model in the PCIe Endpoint and transfer
memory (a fake PDI file) from the QEMU x86 host to the SBI keyhole
aperture address by issueing below commands. In this demo a 4 KB memory
has been connected a the SBI keyhole aperture address.
```
# Create a fake PDI of 1KB size.
$ dd if=/dev/zero of=/home/ubuntu/test.pdi count=1 bs=1024
# Copy a 1KB of a file from the host to the SBI.
$ ./bin/dma-to-device -d /dev/qdma02000-MM-0 -f /home/ubuntu/test.pdi -s 1024 -a 0x102100000
size=1024 Average BW = 31.673125 KB/sec
```
