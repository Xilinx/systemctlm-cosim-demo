QEMU+SystemC DCMAC CoSim Demo using AMD-EDF/25.11 for VPK120 Evaluation board
=============================================================================

# 1. Overview
## 1.1 VPK120
- Board Landing Page</br>
  - <https://www.amd.com/en/products/adaptive-socs-and-fpgas/evaluation-boards/vpk120.html>

- DCMAC design example for VPK120</br>
  - <https://xilinx-wiki.atlassian.net/wiki/pages/viewpage.action?pageId=2554462228#VersalPremiumSeriesVPK120EvaluationKit-DCMACExampleDesign>
  - <https://github.com/Xilinx/Vivado-Design-Tutorials/tree/2023.2/Device_Architecture_Tutorials/Versal/NoC_System_Designs/DCMAC_NoC>

- CoSim Assumptions (based on Linux support):
    - Single port, 400G, 200G, or 100G
    - No 1588 support
    - DCMAC + AXIDMA, supported by prebuilt Linux kernel for VPK120
    - Linux support (more details below in this README)</br>
      <https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842485/Linux+AXI+Ethernet+driver#Features-supported-in-the-driver>

## 1.2 SystemC/TLM-2.0 Simplified Models Implemented in this CoSim Demo
- DCMAC Core Brief: <https://docs.amd.com/r/en-US/ug1273-versal-acap-design/DCMAC>

- DCMAC: <https://docs.amd.com/r/en-US/pg369-dcmac>
  - libsystemctlm-soc/soc/net/ethernet/amd/dcmac/

- AXIDMA: <https://docs.amd.com/r/en-US/pg021_axi_dma>
  - libsystemctlm-soc/soc/dma/amd/axidma/

- AXIGPIO: <https://docs.amd.com/r/en-US/pg144-axi-gpio>
  - ./versal_dcmac_demo.cc

- Top-level Implementation:
  - ./versal_dcmac_demo.cc

## 1.3 High Level Block Diagram
```text
   +----------------------+      +--------------------+                    +-------------------+
   |       <<QEMU>>       |      |      <<QEMU>>      |                    |      <<HOST>>     |
   | VPK120 System Memory | <--> | Aarch64 Versal APU | <-..-..-..-..-..-> |      TCP/UDP      |<-------\
   |      Simulation      |      | with AMD-EDF Linux |  (virtual L7 comm) | Userspace program |        |
   +----------------------+      +--------------------+                    +-------------------+        |
              ^                             ^                                                           |
              |                             |                                                           |
              v                             v                                                           |
   +--------------------+        +--------------------+                                                 |
   |      <<QEMU>>      |        |      <<QEMU>>      |                                                 |
   |   MEM Initiator    |        |     MMIO Target    |                                                 |
   | Remote-port Bridge |        | Remote-port Bridge |                                                 |
   +--------------------+        +--------------------+                                                 |
              ^                             ^                                                           |
              |                             |                                                           |
              |                             +----------------------+---------------------+              |
              |                             |                      |                     |              |
              V                             v                      v                     v              |
   +--------------------+        +--------------------+   +-----------------+   +------------------+    |
   |   <<SYSC.AXIDMA>>  |        |   <<SYSC.AXIDMA>>  |   |  <<SYSC.DCMAC>> |   | <<DEMO.AXIGPIO>> |    |
   |    DMA Initiator   |        |   MMIO Reg Target  |   | MMIO Reg Target |   | MMIO Reg Target  |    |
   +--------------------+        +--------------------+   +-----------------+   +------------------+    |
              ^                             ^                      ^                                    |
              |                             |                      |                                    |
              v                             v                      |                                    |
   +--------------------------------------------------+            |                                    |
   |                   <<SYSC.AXIDMA>>                |            |                                    |
   |             Scatter-Gather DMA Model             |            |                                    |
   +--------------------------------------------------+            |                                    |
                 ^                    |                            |                                    |
                 |                    v                            |                                    |
        +-----------------+   +------------------+                 |                                    |
        | <<SYSC.AXIDMA>> |   | <<SYSC.AXIDMA>>  |                 |                                    |
        | S2MM AXI-Stream |   | MM2S AXI-Stream  |                 |                                    |
        |      Target     |   |    Initiator     |                 |                                    |
        +-----------------+   +------------------+                 |                                    |
                 ^                    |                            |                                    |
                 |                    v                            |                                    |
      +-------------------+   +-------------------+                |                                    |
      |   <<SYSC.DCMAC>>  |   |   <<SYSC.DCMAC>>  |                |                                    |
      | MAC-RX AXI-Stream |   | MAC-TX AXI-Stream |                |                                    |
      |     Initiator     |   |     Target        |                |                                    |
      +-------------------+   +-------------------+                |                                    |
                 ^                    |                            |                                    |
                 |                    v                            |                                    |
      +-------------------------------------------+                |                                    |
      |              <<SYSC.DCMAC>>               |                |                                    |
      |                 FixedE                    |<---------------/                                    |
      |                 Model                     |                                                     |
      +-------------------------------------------+                                                     |
                 ^                    |                                                                 |
                 |                    v                                                                 |
      +-------------------+  +--------------------+                                                     |
      |   <<SYSC.DCMAC>>  |  |   <<SYSC.DCMAC>>   |                                                     |
      |   PHY-RX Target   |  |  PHY-TX Initiator  |                                                     |
      +-------------------+  +--------------------+                                                     |
                 ^                    |                                                                 |
                 |                    v                                                                 |
      +-------------------+  +--------------------+                                                     |
      |      <<QEMU>>     |  |      <<QEMU>>      |                                                     |
      |  remote-port-net  |  |  remote-port-net   |                                                     |
      |      Initiator    |  |      Target        |                                                     |
      +-------------------+  +--------------------+                                                     |
                 ^                    |                                                                 |
                 |                    v                                                                 |
      +-------------------+  +--------------------+                                                     |
      |   <<QEMU.SLiRP>>  |  |  <<QEMU.SLiRP>>    |                                                     |
      |    Ethernet RX    |  |    Ethernet TX     |                                                     |
      +-------------------+  +--------------------+                                                     |
                 ^                    |                                                                 |
                 |                    v                                                                 |
      +-------------------------------------------+                                                     |
      |              <<QEMU.SLiRP>>               |                                                     |
      |  DHCP Server + Eth<=>TCP/UDP Translation  |                                                     |
      |   TCP/UDP Socket-based Host Interaction   |                                                     |
      +-------------------------------------------+                                                     |
                            ^                                                                           |
                            |                                                                           |
                            v                                                                           v
   +--------------------------------------------------------------------------------------------------------+
   |                                                 <<HOST>>                                               |
   |                                      TCP/UDP Network Socket Stack                                      |
   +--------------------------------------------------------------------------------------------------------+
```

## 1.4 Software Components for DCMAC + AXIDMA CoSim Demo
The software components needed for running this demo will be described
in further details later in this document, and here is a summary list:

1. Components built from source codes in this README's repository
   - SystemC Simulation Model for DCMAC + AXIDMA

     This is a subsystem-level model written in SystemC, and the simulation
     will communicate with a virtual VPK120 board over the open-sourced
     AMD/Xilinx Remote-Port protocol for CoSim.

   - Linux device tree nodes for AXIDMA and the DCMAC example design
     The source file can be #include or as a binary overlay

   - A configuration file for running the VPK120 virtual board for CoSim.

   - <mark>BOOT-versal-vpk120-sdt-seg.qemuboot.conf` is a file that
     defines:
     - Names or basename of most other files in the same workspace
     - Arguments required to launch virtual VPK120 board

1. Components as prebuilt binaries from AMD Embedded Development Framework
   - AMD-EDF is a complete open-source environment that helps embedded
     engineers evaluate, develop, and deploy applications on AMD adaptive SoCs.

     See <https://www.amd.com/en/products/software/adaptive-socs-and-fpgas/embedded-software/embedded-development-framework.html>

   - AMD-EDF SDK

     This is a software development kit to work with all components
     provided by AMD-EDF.

     However, this SDK does not have support to build SystemC models.
     The top-level README.md of this document's repository describes
     a build process that is applicable to building the SystemC model
     for this DCMAC + AXIDMA CoSim demo.

   - Virtual VPK120 board implemented by AMD-EDF QEMU

     This is a board-level model written in C within the QEMU framework
     to simulate the VPK120 board.

   - Linux distribution for VPK120

   - VPK120 Boot files

     These are a collection of prebuilt files provided by AMD-EDF to
     initialize and boot the devices on a VPK120 board, and they are
     applicable to physical and virtual VPK120 boards.

Organization of the following contents:

[Section 2](#2-dcmac--axidma-systemc-simulation) describes the SystemC component

[Section 3](#3-amd-edf-amd-embedded-development-framework) describes all AMD-EDF components

[Section 4](#4-running-dcmac--axidma-cosim-demo) describes running this DCMAC + AXIDMA CoSim demo

# 2. DCMAC + AXIDMA SystemC Model

## 2.1 Overview

To run the DCMAC + AXIDMA CoSim demo, it is necessary to build the
SystemC model for a subsystem with DCMAC + AXIDMA, so the subsystem is
readily supported by prebuilt image from the AMD-EDF Linux distribution
for VPK120.

Building the DCMAC + AXIDMA SystemC model consists of these steps:

1. Clone and check out the GIT repo that contains this README into
   /PATH/TO/COSIM_DEMO

1. Go through this repo's top-level README.md to:
   - Install SystemC
   - Configure your SystemC build environment
   - Understand the build process

1. Build the SystemC simulation for Versal DCMAC Demo

1. Locate the SystemC simulation binary</br>
   `/PATH/TO/COSIM_DEMO/versal-dcmac-demo/versal_dcmac_demo`

# 3. AMD-EDF, AMD Embedded Development Framework

This CoSim demo relies on the VPK120 Linux distribution from AMD-EDF,
AMD Embedded Development Framework.

The prebuilt image in the VPK120 Linux distribution from AMD-EDF
has been configured to support DCMAC + AXIDMA out-of-the-box, so
there is no need to modify or rebuild the Linux kernel.

For this CoSim demo, the VPK120 Linux distribution from AMD-EDF
is deployed in a virtual VPK120 board modelled by AMD-EDF QEMU,
a tool that comes with the AMD-EDF SDK.

## 3.1 AMD-EDF Highlights
- AMD-EDF 25.11 release with VPK120 support is accessible to all
  customers, through the AMD-EDF Download Center Landing Page</br>
  <https://www.amd.com/en/support/downloads/adaptive-socs-and-fpgas/embedded-software.html>

> [!TIP] While anyone can browse the content of the landing page, one
> needs to register (sign up) an account to download a file from the
> AMD-EDF Download Center.  The login/sign-up prompt will be displayed
> when a downloadable link is clicked.

> [!TIP] As long as the page used to download the first file remains
> open in a browser, subsequent downloads clicked through the landing
> page will not prompt for login again.

- Getting started with this CoSim demo has the following steps:
  1. Fetch all required files in AMD-EDF.

  1. Install AMD-EDF SDK in a Linux machine (bash assumed here; sudo not needed).

  1. Create a working directory to hold:
     - Prebuilt files extracted from downloaded files.
     - Files copied from this CoSim demo repo

  1. Modify files in the working directory

  1. Launch demo

## 3.2 Fetch Required Files
- Create download directory /PATH/TO/EDF_ARTIFACTS

- Place all downloaded files in /PATH/TO/EDF_ARTIFACTS

- Go through Download Center landing page <https://www.amd.com/en/support/downloads/adaptive-socs-and-fpgas/embedded-software.html> to access the AMD-EDF download site

- AMD-EDF SDK Installer</br>
  - Search for `amd-cortexa72-common_meta-edf-app-sdk` in landing page
  - Click the colored link to start download
  - The downloaded file name is&nbsp;</br>
    `amd-edf-glibc-x86_64-meta-edf-app-sdk-cortexa72-cortexa53-amd-cortexa72-common-toolchain-25.11.1+release-S03202137.sh`

- Prebuilt VPK120 Linux WIC Image (kernel included)
  - Search for `VPK120 EDF Linux BSP` in landing page
  - Click the colored link to start download
  - The downloaded file name is&nbsp;</br>
    `edf-linux-disk-image-amd-cortexa72-common+versal-vpk120-sdt-seg.rootfs-11151020.wic.xz`

## 3.3 Install and Activate AMD-EDF SDK
- Create directory /PATH/TO/EDF_SDK

- Run AMD-EDF SDK installer (no sudo needed):
  ```bash
  /PATH/TO/EDF_ARTIFACTS/amd-edf-glibc-x86_64-meta-edf-app-sdk-cortexa72-cortexa53-amd-cortexa72-common-toolchain-25.11+release-S03202137.sh
  ```
  And follow the prompt to enter your /PATH/TO/EDF_SDK as the install directory

- Activate AMD-EDF SDK (for each shell prompt terminal using the SDK):
  ```bash
  source /PATH/TO/EDF_SDK/environment-setup-cortexa72-cortexa53-amd-linux
  ```

## 3.4 Prepare a VPK120 workspace
It is necessary to create a directory as the workspace to hold files needed
to run the virtual VPK120 board, an important part of this CoSim demo.

For convenience, the entire preparation can be automated as:
```bash
source /PATH/TO/EDF_SDK/environment-setup-cortexa72-cortexa53-amd-linux

cd /PATH/TO/COSIM_DEMO   # Where this repo is
./scripts/BOOT-versal_dcmac_demo-vpk120.setup.sh /PATH/TO/VPK120_DCMAC /PATH/TO/EDF_ARTIFACTS
```

Here are technical details on the preparation.

This workspace holds prebuilt artifacts obtained as described earlier
in this README:

- `BOOT-versal-vpk120-sdt-seg.wic.qemu-sd`
  is an SD-card image with:
  - Linux kernel and rootfs for VPK120

- The AMD-EDF QEMU multi-arch device trees for VPK120

- Files extracted from `BOOT-versal-vpk120-sdt-seg.wic.qemu-sd` and processed

The entire preparation must be carried out in a Linux terminal shell
with AMD-EDF SDK activated for the shell.

### 3.4.1 Prepare the SD-card image file
- The file name must be `BOOT-versal-vpk120-sdt-seg.wic.qemu-sd`
- The file size must be power of 2.

### 3.4.2 Copy in the AMD-EDF QEMU multi-arch device trees
The device tree files are bundled with AMD-EDF SDK, and they are copied
into the VPK120 workspace for modification later.

### 3.4.3 Copy in .qemuboot.conf file
This file is input to AMD-EDF SDK's `runqemu` command, which launches the
virtual VPK120 board implemented by AMD-EDF QEMU.

This is a text file in this CoSim demo repository.

### 3.4.4 Prepare system controller's boot files
These are files needed to boot the system controller of XCVP1200,
the Versal Premium device on the VPK120 board.

For a physical VPK120 board, these files are bundled together in
a file named `/boot.bin` inside the SD-card image.

For a virtual VPK120 board, these files are also needed but must be
unbundled into individual files in a VPK120 workspace directory, using
the `bootgen` command from the AMD-EDF SDK. See file
./script/BOOT-versal_dcmac_demo-vpk120.setup.sh for detail.

### 3.4.5 Test readiness of the workspace
It is strongly recommended that, before making changes to the VPK120
workspace for running this CoSim demo, perform a test-run without
any changes to ensure the workspace has been correctly prepared.

```bash
source /PATH/TO/EDF_SDK/environment-setup-cortexa72-cortexa53-amd-linux

cd /PATH/TO/VPK120_DCMAC
runqemu BOOT-versal-vpk120-sdt-seg.qemuboot.conf nographic slirp snapshot
```

Upon reaching the `amd-edf login:` prompt, enter `amd-edf` to login
to AME-EDF Linux shell.

To end the run:
1. From the AMD-EDF Linux shell, enter `sudo systemctl poweroff`
1. Wait for the "reboot: Power down" message to appear
1. Press Ctrl+A then press the letter key 'x'

### 3.4.6 Details of Linux Support in AMD-EDF
The prebuilt VPK120 Linux distribution provided by AMD-EDF supports
DCMAC + AXIDMA subsystem *out-of-the-box*, so there is no need to
modify or rebuild any part of the Linux distribution.

Here are the relevant details regarding the support:
- Kconfig selector: XILINX_AXI_EMAC
- Driver module name: xilinx_emac.mod
- Linux DTB binding: <https://github.com/Xilinx/linux-xlnx/blob/master/Documentation/devicetree/bindings/net/xlnx%2Caxi-ethernet.yaml#L69>
- Driver source: <https://github.com/Xilinx/linux-xlnx/blob/xilinx-v2025.2/drivers/net/ethernet/xilinx/xilinx_axienet_main.c#L4963>

The prebuilt Linux kernel image is stored in</br>
`/PATH/TO/VPK120_DCMAC/BOOT-versal-vpk120-sdt-seg.wic.qemu-sd`,
and its built configuration can be confirmed as follows:
```bash
source /PATH/TO/EDF_SDK/environment-setup-cortexa72-cortexa53-amd-linux

cd /PATH/TO/VPK120_DCMAC
wic cp BOOT-versal-vpk120-sdt-seg.wic.qemu-sd:1/Image Image

mkdir -p /PATH/TO/MY_XLNX_LINUX
cd /PATH/TO/MY_XLNX_LINUX
git clone https://github.com/Xilinx/linux-xlnx.git .
git checkout -t origin/xilinx-v2025.2
./scripts/extract-ikconfig /PATH/TO/VPK120_DCMAC/Image | egrep 'AXI_EMAC|HAS_MCDMA'
```

And the output looks like:
```text
CONFIG_XILINX_AXI_EMAC=y
# CONFIG_XILINX_AXI_EMAC_HWTSTAMP is not set
# CONFIG_AXIENET_HAS_MCDMA is not set
```

# 4. Running DCMAC + AXIDMA CoSim Demo
1. Open 2 separate terminal shell

1. From 1st shell (commands in this shell must run first),
   ```bash
   source /PATH/TO/EDF_SDK/environment-setup-cortexa72-cortexa53-amd-linux

   cd /PATH/TO/VPK120_DCMAC
   rm -f ./dcmac-cosim.sock
   runqemu BOOT-versal_dcmac_demo-vpk120.qemuboot.conf nographic slirp qemuparams="-chardev socket,id=pl-rp,path=./dcmac-cosim.sock,server=on"
   ```

1. From 2nd shell (commands in this shell cannot run first),
   ```bash
   export DCMAC_DEMO_DEBUG=1  # optional
   /PATH/TO/COSIM_DEMO/versal_dcmac_demo unix:/PATH/TO/VPK120_DCMAC/dcmac-cosim.sock 10000
   ```   

1. Return to 1st shell,
   1. Wait for Linux login prompt to appear.
   1. Log in using username amd-edf
   1. Upon reaching the shell prompt, enter command: ip route show

   Look in the command output for a line starting with 192.168.8.0/24.
   Its presence indicates that DCMAC has successfully exchanged packets
   with QEMU user-mode network's DHCP server using the DHCP protocol.

   Here is an output example:
   ```text
   amd-edf:~$ ip route show
   default via 192.168.8.2 dev eth1 proto dhcp src 192.168.8.15 metric 10 
   default via 10.0.2.2 dev end0 proto dhcp src 10.0.2.15 metric 10 
   10.0.2.0/24 dev end0 proto kernel scope link src 10.0.2.15 metric 10 
   10.0.2.2 dev end0 proto dhcp scope link src 10.0.2.15 metric 10 
   10.0.2.3 dev end0 proto dhcp scope link src 10.0.2.15 metric 10 
   172.17.0.0/16 dev docker0 proto kernel scope link src 172.17.0.1 linkdown 
   192.168.8.0/24 dev eth1 proto kernel scope link src 192.168.8.15 metric 10 
   192.168.8.2 dev eth1 proto dhcp scope link src 192.168.8.15 metric 10 
   192.168.8.3 dev eth1 proto dhcp scope link src 192.168.8.15 metric 10 
   amd-edf:~$ 
   ```
