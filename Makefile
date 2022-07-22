#
# Cosim Makefiles
#
# Copyright (c) 2016 Xilinx Inc.
# Written by Edgar E. Iglesias
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

-include .config.mk

INSTALL ?= install

ifneq "$(VCS_HOME)" ""
SYSTEMC_INCLUDE ?=$(VCS_HOME)/include/systemc232/
SYSTEMC_LIBDIR ?= $(VCS_HOME)/linux/lib
TLM2 ?= $(VCS_HOME)/etc/systemc/tlm/

HAVE_VERILOG=y
HAVE_VERILOG_VERILATOR?=n
HAVE_VERILOG_VCS=y
else
SYSTEMC ?= /usr/local/systemc-2.3.2/
SYSTEMC_INCLUDE ?=$(SYSTEMC)/include/
SYSTEMC_LIBDIR ?= $(SYSTEMC)/lib-linux64
# In case your TLM-2.0 installation is not bundled with
# with the SystemC one.
# TLM2 ?= /opt/systemc/TLM-2009-07-15
endif

SCML ?= /usr/local/scml-2.3/
SCML_INCLUDE ?= $(SCML)/include/
SCML_LIBDIR ?= $(SCML)/lib-linux64/

HAVE_VERILOG?=n
HAVE_VERILOG_VERILATOR?=n
HAVE_VERILOG_VCS?=n

CFLAGS += -Wall -O2 -g
CXXFLAGS += -Wall -O2 -g

ifneq "$(SYSTEMC_INCLUDE)" ""
CPPFLAGS += -I $(SYSTEMC_INCLUDE)
endif
ifneq "$(TLM2)" ""
CPPFLAGS += -I $(TLM2)/include/tlm
endif

CPPFLAGS += -I .
LDFLAGS  += -L $(SYSTEMC_LIBDIR)
#LDLIBS += -pthread -Wl,-Bstatic -lsystemc -Wl,-Bdynamic
LDLIBS   += -pthread -lsystemc

ZYNQ_TOP_C = zynq_demo.cc
ZYNQ_TOP_O = $(ZYNQ_TOP_C:.cc=.o)
ZYNQMP_TOP_C = zynqmp_demo.cc
ZYNQMP_TOP_O = $(ZYNQMP_TOP_C:.cc=.o)
ZYNQMP_LMAC2_TOP_C = zynqmp_lmac2_demo.cc
ZYNQMP_LMAC2_TOP_O = $(ZYNQMP_LMAC2_TOP_C:.cc=.o)
RISCV_VIRT_LMAC2_TOP_C = riscv_virt_lmac2_demo.cc
RISCV_VIRT_LMAC2_TOP_O = $(RISCV_VIRT_LMAC2_TOP_C:.cc=.o)
RISCV_VIRT_LMAC3_TOP_C = riscv_virt_lmac3_demo.cc
RISCV_VIRT_LMAC3_TOP_O = $(RISCV_VIRT_LMAC3_TOP_C:.cc=.o)
VERSAL_MRMAC_TOP_C = versal_mrmac_demo.cc
VERSAL_MRMAC_TOP_O = $(VERSAL_MRMAC_TOP_C:.cc=.o)
VERSAL_TOP_C = versal_demo.cc
VERSAL_TOP_O = $(VERSAL_TOP_C:.cc=.o)
PCIE_ATS_DEMO_C = pcie-ats-demo/pcie-ats-demo.cc
PCIE_ATS_DEMO_O = $(PCIE_ATS_DEMO_C:.cc=.o)
TEST_PCIE_ATS_DEMO_VFIO_C = pcie-ats-demo/test-pcie-ats-demo-vfio.o
TEST_PCIE_ATS_DEMO_VFIO_O = $(TEST_PCIE_ATS_DEMO_VFIO_C:.cc=.o)
PCIE_ACC_MD5SUM_VFIO_C = pcie-ats-demo/pcie-acc-md5sum-vfio.o
PCIE_ACC_MD5SUM_VFIO_O = $(PCIE_ACC_MD5SUM_VFIO_C:.cc=.o)
VERSAL_NET_CDX_STUB_C = versal_net_cdx_stub.cc
VERSAL_NET_CDX_STUB_O = $(VERSAL_NET_CDX_STUB_C:.cc=.o)

ZYNQ_OBJS += $(ZYNQ_TOP_O)
ZYNQMP_OBJS += $(ZYNQMP_TOP_O)
ZYNQMP_LMAC2_OBJS += $(ZYNQMP_LMAC2_TOP_O)
VERSAL_MRMAC_OBJS += $(VERSAL_MRMAC_TOP_O)
RISCV_VIRT_LMAC2_OBJS += $(RISCV_VIRT_LMAC2_TOP_O)
RISCV_VIRT_LMAC3_OBJS += $(RISCV_VIRT_LMAC3_TOP_O)
VERSAL_OBJS += $(VERSAL_TOP_O)
PCIE_ATS_DEMO_OBJS += $(PCIE_ATS_DEMO_O)
TEST_PCIE_ATS_DEMO_VFIO_OBJS += $(TEST_PCIE_ATS_DEMO_VFIO_O)
PCIE_ACC_MD5SUM_VFIO_OBJS += $(PCIE_ACC_MD5SUM_VFIO_O)
VERSAL_NET_CDX_STUB_OBJS += $(VERSAL_NET_CDX_STUB_O)

# Uncomment to enable use of scml2
# CPPFLAGS += -I $(SCML_INCLUDE)
# LDFLAGS += -L $(SCML_LIBDIR)
# LDLIBS += -lscml2 -lscml2_logging

SC_OBJS += trace.o
SC_OBJS += debugdev.o
SC_OBJS += demo-dma.o
SC_OBJS += xilinx-axidma.o

LIBSOC_PATH=libsystemctlm-soc
CPPFLAGS += -I $(LIBSOC_PATH)

LIBSOC_ZYNQ_PATH=$(LIBSOC_PATH)/soc/xilinx/zynq
SC_OBJS += $(LIBSOC_ZYNQ_PATH)/xilinx-zynq.o
CPPFLAGS += -I $(LIBSOC_ZYNQ_PATH)

LIBSOC_ZYNQMP_PATH=$(LIBSOC_PATH)/soc/xilinx/zynqmp
SC_OBJS += $(LIBSOC_ZYNQMP_PATH)/xilinx-zynqmp.o
CPPFLAGS += -I $(LIBSOC_ZYNQMP_PATH)

CPPFLAGS += -I $(LIBSOC_PATH)/soc/xilinx/versal/
SC_OBJS += $(LIBSOC_PATH)/soc/xilinx/versal/xilinx-versal.o

CPPFLAGS += -I $(LIBSOC_PATH)/soc/xilinx/versal-net/
SC_OBJS += $(LIBSOC_PATH)/soc/xilinx/versal-net/xilinx-versal-net.o

CPPFLAGS += -I $(LIBSOC_PATH)/tests/test-modules/
SC_OBJS += $(LIBSOC_PATH)/tests/test-modules/memory.o

LIBRP_PATH=$(LIBSOC_PATH)/libremote-port
C_OBJS += $(LIBRP_PATH)/safeio.o
C_OBJS += $(LIBRP_PATH)/remote-port-proto.o
C_OBJS += $(LIBRP_PATH)/remote-port-sk.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-master.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-slave.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-wires.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-ats.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-pci-ep.o
SC_OBJS += $(LIBSOC_PATH)/soc/pci/core/pci-device-base.o
SC_OBJS += $(LIBSOC_PATH)/soc/dma/xilinx/mcdma/mcdma.o
SC_OBJS += $(LIBSOC_PATH)/soc/net/ethernet/xilinx/mrmac/mrmac.o
CPPFLAGS += -I $(LIBRP_PATH)

VENV=SYSTEMC_INCLUDE=$(SYSTEMC_INCLUDE) SYSTEMC_LIBDIR=$(SYSTEMC_LIBDIR)
VOBJ_DIR=obj_dir
VFILES=apb_timer.v

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
VERILATOR_ROOT?=/usr/share/verilator
VERILATOR=verilator

VM_SC?=1
VM_TRACE?=0
VM_COVERAGE?=0
V_LDLIBS += $(VOBJ_DIR)/Vapb_timer__ALL.a
V_LDLIBS += $(VOBJ_DIR)/Vaxilite_dev__ALL.a
V_LDLIBS += $(VOBJ_DIR)/Vaxifull_dev__ALL.a
LDLIBS += $(V_LDLIBS)
VERILATED_O=$(VOBJ_DIR)/verilated.o

# Gives some compatibility with vcs
VFLAGS += --pins-bv 2 -Wno-fatal
VFLAGS += --output-split-cfuncs 500

VFLAGS+=--sc --Mdir $(VOBJ_DIR)
VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
CPPFLAGS += -DHAVE_VERILOG
CPPFLAGS += -DHAVE_VERILOG_VERILATOR
CPPFLAGS += -I $(VOBJ_DIR)

ifeq "$(VM_TRACE)" "1"
VFLAGS += --trace
SC_OBJS += verilated_vcd_c.o
SC_OBJS += verilated_vcd_sc.o
CPPFLAGS += -DVM_TRACE=1
endif
endif

ifeq "$(HAVE_VERILOG_VCS)" "y"
VCS=vcs
SYSCAN=syscan
VLOGAN=vlogan
VHDLAN=vhdlan

CSRC_DIR = csrc

VLOGAN_FLAGS += -sysc
VLOGAN_FLAGS += +v2k -sc_model apb_slave_timer

VHDLAN_FLAGS += -sysc
VHDLAN_FLAGS += -sc_model apb_slave_dummy

SYSCAN_ZYNQ_DEMO = zynq_demo.cc
SYSCAN_ZYNQMP_DEMO = zynqmp_demo.cc
SYSCAN_ZYNQMP_LMAC2_DEMO = zynqmp_lmac2_demo.cc
SYSCAN_SCFILES += demo-dma.cc debugdev.cc remote-port-tlm.cc
VCS_CFILES += remote-port-proto.c remote-port-sk.c safeio.c

SYSCAN_FLAGS += -tlm2 -sysc=opt_if
SYSCAN_FLAGS += -cflags -DHAVE_VERILOG -cflags -DHAVE_VERILOG_VCS
VCS_FLAGS += -sysc sc_main -sysc=adjust_timeres
VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
endif

OBJS = $(C_OBJS) $(SC_OBJS)

ZYNQ_OBJS += $(OBJS)
ZYNQMP_OBJS += $(OBJS)
ZYNQMP_LMAC2_OBJS += $(OBJS)
VERSAL_MRMAC_OBJS += $(OBJS)
RISCV_VIRT_LMAC2_OBJS += $(OBJS)
RISCV_VIRT_LMAC3_OBJS += $(OBJS)
VERSAL_OBJS += $(OBJS)
PCIE_ATS_DEMO_OBJS += $(OBJS)
VERSAL_NET_CDX_STUB_OBJS += $(OBJS)

TARGET_ZYNQ_DEMO = zynq_demo
TARGET_ZYNQMP_DEMO = zynqmp_demo
TARGET_ZYNQMP_LMAC2_DEMO = zynqmp_lmac2_demo
TARGET_VERSAL_MRMAC_DEMO = versal_mrmac_demo
TARGET_RISCV_VIRT_LMAC2_DEMO = riscv_virt_lmac2_demo
TARGET_RISCV_VIRT_LMAC3_DEMO = riscv_virt_lmac3_demo
TARGET_VERSAL_DEMO = versal_demo
TARGET_PCIE_ATS_DEMO = pcie-ats-demo/pcie-ats-demo
TARGET_TEST_PCIE_ATS_DEMO_VFIO = pcie-ats-demo/test-pcie-ats-demo-vfio
TARGET_VERSAL_NET_CDX_STUB = versal_net_cdx_stub
PCIE_ACC_MD5SUM_VFIO = pcie-ats-demo/pcie-acc-md5sum-vfio

IPXACT_LIBS = packages/ipxact
DEMOS_IPXACT_LIB = $(IPXACT_LIBS)/xilinx.com/demos
ZL_IPXACT_DEMO_DIR = $(DEMOS_IPXACT_LIB)/zynqmp_lmac2_demo/1.0
ZL_IPXACT_DEMO = $(ZL_IPXACT_DEMO_DIR)/zynqmp_lmac2_demo.1.0.xml
ZL_IPXACT_DEMO_OUTDIR = zynqmp_lmac2_ipxact_demo
TARGET_ZYNQMP_LMAC2_IPXACT_DEMO = $(ZL_IPXACT_DEMO_OUTDIR)/sc_sim

PYSIMGEN = $(LIBSOC_PATH)/tools/pysimgen/pysimgen
PYSIMGEN_ARGS = -p $(ZL_IPXACT_DEMO)
PYSIMGEN_ARGS += -l $(IPXACT_LIBS) $(LIBSOC_PATH)/$(IPXACT_LIBS)
PYSIMGEN_ARGS += -o $(ZL_IPXACT_DEMO_OUTDIR)
PYSIMGEN_ARGS += --build --quiet

TARGETS = $(TARGET_ZYNQ_DEMO) $(TARGET_ZYNQMP_DEMO) $(TARGET_VERSAL_DEMO) $(TARGET_VERSAL_MRMAC_DEMO)
TARGETS += $(TARGET_VERSAL_NET_CDX_STUB)

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
#
# LMAC2
#
LM2_DIR=LMAC_CORE2/LMAC2_INFO/

LM_CORE = lmac_wrapper_top.v
include files-lmac2.mk
ifneq ($(wildcard $(LM2_DIR)/.),)
TARGETS += $(TARGET_ZYNQMP_LMAC2_DEMO)
TARGETS += $(TARGET_RISCV_VIRT_LMAC2_DEMO)
V_LDLIBS += $(VOBJ_DIR)/Vlmac_wrapper_top__ALL.a
ifneq ($(wildcard $(PYSIMGEN)),)
TARGETS += $(TARGET_ZYNQMP_LMAC2_IPXACT_DEMO)
endif
endif
endif

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
#
# LMAC3
#
LM3_DIR=LMAC_CORE3/LMAC3_INFO/

LM3_CORE = lmac3_wrapper_top.v
include files-lmac3.mk
ifneq ($(wildcard $(LM3_DIR)/.),)
TARGETS += $(TARGET_RISCV_VIRT_LMAC3_DEMO)
V_LDLIBS += $(VOBJ_DIR)/Vlmac3_wrapper_top__ALL.a
endif
endif

all: $(TARGETS)

-include $(ZYNQ_OBJS:.o=.d)
-include $(ZYNQMP_OBJS:.o=.d)
-include $(ZYNQMP_LMAC2_OBJS:.o=.d)
-include $(VERSAL_MRMAC_OBJS:.o=.d)
-include $(RISCV_VIRT_LMAC2_OBJS:.o=.d)
-include $(RISCV_VIRT_LMAC3_OBJS:.o=.d)
-include $(VERSAL_OBJS:.o=.d)
-include $(PCIE_ATS_DEMO_OBJS:.o=.d)
-include $(TEST_PCIE_ATS_DEMO_VFIO_OBJS:.o=.d)
-include $(PCIE_ACC_MD5SUM_VFIO_OBJS:.o=.d)
CFLAGS += -MMD
CXXFLAGS += -MMD

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
include $(VERILATOR_ROOT)/include/verilated.mk

$(VOBJ_DIR)/Vlmac_wrapper_top__ALL.a: $(LM_CORE)
	$(VENV) $(VERILATOR) $(VFLAGS) $^
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f Vlmac_wrapper_top.mk

$(VOBJ_DIR)/Vlmac3_wrapper_top__ALL.a: $(LM3_CORE)
	$(VENV) $(VERILATOR) $(VFLAGS) $^
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f Vlmac3_wrapper_top.mk

$(ZYNQMP_LMAC2_TOP_O): $(V_LDLIBS)
$(ZYNQMP_LMAC3_TOP_O): $(V_LDLIBS)
$(RISCV_VIRT_LMAC2_TOP_O): $(V_LDLIBS)
$(RISCV_VIRT_LMAC3_TOP_O): $(V_LDLIBS)
$(ZYNQMP_TOP_O): $(V_LDLIBS)
$(VERILATED_O): $(V_LDLIBS)

$(VOBJ_DIR)/V%__ALL.a: %.v
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f V$(<:.v=.mk)
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f V$(<:.v=.mk) verilated.o

EX_AXI4LITE_PATH = $(LIBSOC_PATH)/tests/example-rtl-axi4lite
AXILITE_DEV_V = axilite_dev.v
VFLAGS += -y $(EX_AXI4LITE_PATH)

$(VOBJ_DIR)/Vaxilite_dev__ALL.a: $(EX_AXI4LITE_PATH)/$(AXILITE_DEV_V)
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f V$(AXILITE_DEV_V:.v=.mk)

EX_AXI4_PATH = $(LIBSOC_PATH)/tests/example-rtl-axi4
AXIFULL_DEV_V = axifull_dev.v
VFLAGS += -y $(EX_AXI4_PATH)

$(VOBJ_DIR)/Vaxifull_dev__ALL.a: $(EX_AXI4_PATH)/$(AXIFULL_DEV_V)
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) CXXFLAGS="$(CXXFLAGS)" -f V$(AXIFULL_DEV_V:.v=.mk)
endif

ifeq "$(HAVE_VERILOG_VCS)" "y"
$(TARGET_ZYNQMP_DEMO): $(VFILES) $(SYSCAN_SCFILES) $(VCS_CFILES) $(SYSCAN_ZYNQMP_DEMO)
	$(VLOGAN) $(VLOGAN_FLAGS) $(VFILES)
	$(SYSCAN) $(SYSCAN_FLAGS) $(SYSCAN_ZYNQMP_DEMO) $(SYSCAN_SCFILES)
	$(VCS) $(VCS_FLAGS) $(VFLAGS) $(VCS_CFILES) -o $@
else

$(TARGET_ZYNQMP_DEMO): $(ZYNQMP_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_ZYNQMP_LMAC2_DEMO): $(ZYNQMP_LMAC2_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_VERSAL_MRMAC_DEMO): $(VERSAL_MRMAC_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_ZYNQMP_LMAC2_IPXACT_DEMO):
	$(INSTALL) -d $(ZL_IPXACT_DEMO_OUTDIR)
	[ ! -e .config.mk ] || $(INSTALL) .config.mk $(ZL_IPXACT_DEMO_OUTDIR)
	$(PYSIMGEN) $(PYSIMGEN_ARGS)

$(TARGET_RISCV_VIRT_LMAC2_DEMO): $(RISCV_VIRT_LMAC2_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_RISCV_VIRT_LMAC3_DEMO): $(RISCV_VIRT_LMAC3_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

endif

$(TARGET_ZYNQ_DEMO): $(ZYNQ_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TARGET_VERSAL_DEMO): $(VERSAL_OBJS) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $(VERSAL_OBJS) $(VERILATED_O) $(LDLIBS)

$(TARGET_PCIE_ATS_DEMO): LDLIBS += -lcrypto
$(TARGET_PCIE_ATS_DEMO): $(PCIE_ATS_DEMO_OBJS) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $(PCIE_ATS_DEMO_OBJS) $(VERILATED_O) $(LDLIBS)

$(TARGET_TEST_PCIE_ATS_DEMO_VFIO): $(TEST_PCIE_ATS_DEMO_VFIO_OBJS) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $(TEST_PCIE_ATS_DEMO_VFIO_OBJS) $(LDLIBS)

$(PCIE_ACC_MD5SUM_VFIO): $(PCIE_ACC_MD5SUM_VFIO_OBJS) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $(PCIE_ACC_MD5SUM_VFIO_OBJS) $(LDLIBS)

$(TARGET_VERSAL_NET_CDX_STUB): $(VERSAL_NET_CDX_STUB_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

verilated_%.o: $(VERILATOR_ROOT)/include/verilated_%.cpp

clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGETS)
	$(RM) $(ZYNQ_OBJS) $(ZYNQ_OBJS:.o=.d)
	$(RM) $(ZYNQMP_OBJS) $(ZYNQMP_OBJS:.o=.d)
	$(RM) $(ZYNQMP_LMAC2_OBJS) $(ZYNQMP_LMAC2_OBJS:.o=.d)
	$(RM) $(VERSAL_OBJS) $(VERSAL_OBJS:.o=.d)
	$(RM) $(VERSAL_MRMAC_OBJS) $(VERSAL_MRMAC_OBJS:.o=.d)
	$(RM) $(RISCV_VIRT_LMAC2_OBJS) $(RISCV_VIRT_LMAC2_OBJS:.o=.d)
	$(RM) $(RISCV_VIRT_LMAC3_OBJS) $(RISCV_VIRT_LMAC3_OBJS:.o=.d)
	$(RM) -r $(VOBJ_DIR) $(CSRC_DIR) *.daidir
	$(RM) -r $(ZL_IPXACT_DEMO_OUTDIR)
	$(RM) $(PCIE_ATS_DEMO_OBJS) $(PCIE_ATS_DEMO_OBJS:.o=.d)
	$(RM) $(TEST_PCIE_ATS_DEMO_VFIO_OBJS) $(TEST_PCIE_ATS_DEMO_VFIO_OBJS:.o=.d)
	$(RM) $(PCIE_ACC_MD5SUM_VFIO_OBJS) $(PCIE_ACC_MD5SUM_VFIO_OBJS:.o=.d)
	$(RM) $(TARGET_PCIE_ATS_DEMO) $(TARGET_TEST_PCIE_ATS_DEMO_VFIO)
	$(RM) $(PCIE_ACC_MD5SUM_VFIO)
	$(RM) $(VERSAL_NET_CDX_STUB_OBJS) $(VERSAL_NET_CDX_STUB_OBJS:.o=.d)
	$(RM) $(TARGET_VERSAL_NET_CDX_STUB)
