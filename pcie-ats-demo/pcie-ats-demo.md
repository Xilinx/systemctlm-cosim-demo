# Launching the pcie-ats-demo

## Overview

This demo shows an x86 host co-simulating with an attached PCIe MD5 accelerator.
The x86 host is emulated by QEMU and the PCIe End-point is modelled in
SystemC.

The PCIe MD5 accelerator contains an Address Translation Cache (ATC) and will
issue PCIe Address Translation Service (ATS) requests towards the x86 host
in order to get hold of the Virtual to Physical Adresses translations in
the hosts IOMMU. This allows the MD5 accelerator to later issue memory
transactions using already translated addresses.

Instructions for how to run an Ubuntu based guest system with Xilinx QEMU
together with the pcie-ats-demo demo are found below.

## Building the pcie-ats-demo

The pcie-ats-demo also depends on the libssl-dev Ubuntu/debian package (or
equivalent package in case a different distribution is running on the host)
besides the dependencies mentioned in the README found in the top directory.
Below are instructions on how to install the package on a Ubuntu / debian
system.

```
# Install libssl-dev
$ sudo apt-get install libssl-dev
```

Once above package has been installed (and the README in the top directory has
been followed regarding the SystemC installation and also configuration of the
systemctlm-cosim-demo), one should be able to build the demo with the commands
found below.

```
# Build the pcie-ats-demo
$ cd /path/to/systemctlm-cosim-demo
$ make pcie-ats-demo/pcie-ats-demo
```

## Preparing the Ubuntu cloud image VM

Download the the Ubuntu cloud image.

```
~$ cd ~/Downloads/
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/ubuntu-20.04-server-cloudimg-amd64.img
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/unpacked/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic
~$ wget https://cloud-images.ubuntu.com/releases/focal/release-20210125/unpacked/ubuntu-20.04-server-cloudimg-amd64-initrd-generic
```

Resize the image to 10G.

```
~$ qemu-img resize ~/Downloads/ubuntu-20.04-server-cloudimg-amd64.img 10G
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
$ mkdir /tmp/machine-x86
$ qemu-system-x86_64                                                              \
    -M q35,accel=kvm,kernel-irqchip=split -m 4G -smp 4 -enable-kvm                \
    -device virtio-net-pci,netdev=net0 -netdev type=user,id=net0                  \
    -serial mon:stdio -machine-path /tmp/machine-x86 -display none                \
    -device intel-iommu,intremap=on,device-iotlb=on                               \
    -device ioh3420,id=rootport,slot=0 -device ioh3420,id=rootport1,slot=1        \
    -drive file=~/Downloads/ubuntu-20.04-server-cloudimg-amd64.img,format=qcow2   \
    -drive file=~/Downloads/user-data.img,format=raw                              \
    -kernel ~/Downloads/ubuntu-20.04-server-cloudimg-amd64-vmlinuz-generic        \
    -append "root=/dev/sda1 ro console=tty1 console=ttyS0 intel_iommu=on"         \
    -initrd ~/Downloads/ubuntu-20.04-server-cloudimg-amd64-initrd-generic
```

Above command provides network access through a virtio-net-pci device. Once the
system has booted login with the user 'ubuntu' and password 'pass'.

## Required packages in the Ubuntu cloud image

Login into the Ubuntu cloud image and install below packages:
```
$ sudo apt-get update
$ sudo apt-get install git build-essential autoconf flex bison libssl-dev
```

## Install SystemC in the Ubuntu cloud image

Download and install SystemC 2.3.2 by issuing the following commands in
the VM:

```
$ mkdir ~/Downloads
$ cd ~/Downloads/
$ wget http://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.2.tar.gz
$ tar xzf systemc-2.3.2.tar.gz
$ cd systemc-2.3.2
$ ./configure --prefix=/opt/systemc-2.3.2
$ make
$ sudo make install
```

## Build the pcie-ats-demo's VFIO applications in the Ubuntu cloud image

Clone and build systemctlm-cosim-demo's pcie-ats-demo applications by issuing
the following commands in the VM:

```
$ mkdir ~/github/
$ cd ~/github/
$ git clone https://github.com/Xilinx/systemctlm-cosim-demo.git
$ cd systemctlm-cosim-demo
$ cat <<EOF > .config.mk
SYSTEMC = /opt/systemc-2.3.2/
EOF
$ git submodule update --init libsystemctlm-soc
$ make pcie-ats-demo/test-pcie-ats-demo-vfio pcie-ats-demo/pcie-acc-md5sum-vfio
```

## Connecting QEMU and with the pcie-ats-demo

Launch the pcie-ats-demo in another terminal:

```
$ ./pcie-ats-demo/pcie-ats-demo unix:/tmp/machine-x86/qemu-rport-_machine_peripheral_rp0_rp 10000
```

Press ctrl+a + c in QEMU's terminal to enter the monitor and instantiate a
remote-port adaptor and also hotplug the PCIe EP:

```
device_add remote-port-pci-adaptor,bus=rootport1,id=rp0
device_add remote-port-pci-device,bus=rootport,rp-adaptor0=rp,rp-chan0=0,vendor-id=0x10ee,device-id=0xd004,class-id=0x0700,revision=0x12,nr-io-bars=0,nr-mm-bars=1,bar-size0=0x100000,id=pcidev1,ats=true

```

More information about how to hotplug PCIe EPs into QEMU can be found inside
the libsystemctlm-soc's documentation (see
libsystemctlm-soc/docs/pcie-rtl-bridges/overview.md).

## Running VFIO demo applications

After connecting the pcie-ats-demo EP with QEMU one can verify that
the Ubuntu guest system and Linux kernel has found the device with the
following command:

```
$ sudo lspci -v
...
01:00.0 Serial controller: Xilinx Corporation Device d004 (rev 12) (prog-if 01 [16450])
        Subsystem: Red Hat, Inc. Device 1100
        Physical Slot: 0
        Flags: fast devsel, IRQ 23
        Memory at fe800000 (32-bit, non-prefetchable) [size=1M]
        Capabilities: [40] Express Endpoint, MSI 00
        Capabilities: [100] Address Translation Service (ATS)
...
```

For running the VFIO demo applications below commands can be used (more
detailed information about the commands can be found inside libsystemctlm-soc).

The following VFIO demo application will compute the MD5 digest for a given file,
in this case the Makefile, using the PCIe MD5 accelerator. This can be compared
with the results of the hosts md5sum application.

```
$ cd ~/github/systemctlm-cosim-demo/
$ sudo modprobe vfio-pci nointxmask=1
$ sudo sh -c 'echo 10ee d004 > /sys/bus/pci/drivers/vfio-pci/new_id'
$ # Find the iommu group
$ ls -l /sys/bus/pci/devices/0000\:01\:00.0/iommu_group
$ # The iommu group was 3, MD5 checksum will be calculated on the 'Makefile'
$ sudo LD_LIBRARY_PATH=/opt/systemc-2.3.2/lib-linux64/ ./pcie-ats-demo/pcie-acc-md5sum-vfio 0000:01:00.0 3 Makefile

        SystemC 2.3.2-Accellera --- Mar 30 2021 14:55:02
        Copyright (c) 1996-2017 by all Contributors,
        ALL RIGHTS RESERVED
Device supports 9 regions, 5 irqs
mapped 0 at 0x7f21b9835000

 * MD5: Makefile
   - MD5 result: 605639aa8252d495f5273e1df83110aa

Info: /OSCI/SystemC: Simulation stopped by user.
```
