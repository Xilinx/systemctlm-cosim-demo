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

ifneq "$(VCS_HOME)" ""
SYSTEMC_INCLUDE ?=$(VCS_HOME)/include/systemc23/
SYSTEMC_LIBDIR ?= $(VCS_HOME)/linux/lib
TLM2 ?= $(VCS_HOME)/etc/systemc/tlm/

HAVE_VERILOG=y
HAVE_VERILOG_VERILATOR?=n
HAVE_VERILOG_VCS=y
else
SYSTEMC ?= /usr/local/systemc-2.3.1/
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
LDFLAGS = -L $(SYSTEMC_LIBDIR)
LDLIBS   += -lsystemc

ZYNQ_TOP_C = zynq_demo.cc
ZYNQ_TOP_O = $(ZYNQ_TOP_C:.cc=.o)
ZYNQMP_TOP_C = zynqmp_demo.cc
ZYNQMP_TOP_O = $(ZYNQMP_TOP_C:.cc=.o)
ZYNQMP_LMAC2_TOP_C = zynqmp_lmac2_demo.cc
ZYNQMP_LMAC2_TOP_O = $(ZYNQMP_LMAC2_TOP_C:.cc=.o)

ZYNQ_OBJS += $(ZYNQ_TOP_O)
ZYNQMP_OBJS += $(ZYNQMP_TOP_O)
ZYNQMP_LMAC2_OBJS += $(ZYNQMP_LMAC2_TOP_O)

# Uncomment to enable use of scml2
# CPPFLAGS += -I $(SCML_INCLUDE)
# LDFLAGS += -L $(SCML_LIBDIR)
# LDLIBS += -lscml2 -lscml2_logging

SC_OBJS += memory.o
SC_OBJS += trace.o
SC_OBJS += debugdev.o
SC_OBJS += demo-dma.o
SC_OBJS += xilinx-axidma.o

LIBSOC_PATH=libsystemctlm-soc
CPPFLAGS += -I $(LIBSOC_PATH)

LIBSOC_ZYNQ_PATH=$(LIBSOC_PATH)/zynq
SC_OBJS += $(LIBSOC_ZYNQ_PATH)/xilinx-zynq.o
CPPFLAGS += -I $(LIBSOC_ZYNQ_PATH)

LIBSOC_ZYNQMP_PATH=$(LIBSOC_PATH)/zynqmp
SC_OBJS += $(LIBSOC_ZYNQMP_PATH)/xilinx-zynqmp.o
CPPFLAGS += -I $(LIBSOC_ZYNQMP_PATH)

LIBRP_PATH=$(LIBSOC_PATH)/libremote-port
C_OBJS += $(LIBRP_PATH)/safeio.o
C_OBJS += $(LIBRP_PATH)/remote-port-proto.o
C_OBJS += $(LIBRP_PATH)/remote-port-sk.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-master.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-slave.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-wires.o
CPPFLAGS += -I $(LIBRP_PATH)

VENV=SYSTEMC_INCLUDE=$(SYSTEMC_INCLUDE) SYSTEMC_LIBDIR=$(SYSTEMC_LIBDIR)
VOBJ_DIR=obj_dir
VFILES=apb_timer.v

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
VERILATOR_ROOT?=/usr/share/verilator
VERILATOR=verilator

VM_TRACE?=0
VM_COVERAGE?=0
V_LDLIBS += $(VOBJ_DIR)/Vapb_timer__ALL.a
V_LDLIBS += $(VOBJ_DIR)/Vaxilite_dev_v1_0__ALL.a
V_LDLIBS += $(VOBJ_DIR)/Vaxifull_dev_v1_0__ALL.a
LDLIBS += $(V_LDLIBS)
VERILATED_O=$(VOBJ_DIR)/verilated.o

# Gives some compatibility with vcs
VFLAGS += --pins-bv 2 -Wno-fatal

VFLAGS+=--sc --Mdir $(VOBJ_DIR)
VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
CPPFLAGS += -DHAVE_VERILOG
CPPFLAGS += -DHAVE_VERILOG_VERILATOR
CPPFLAGS += -I $(VOBJ_DIR)
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

TARGET_ZYNQ_DEMO = zynq_demo
TARGET_ZYNQMP_DEMO = zynqmp_demo
TARGET_ZYNQMP_LMAC2_DEMO = zynqmp_lmac2_demo

TARGETS = $(TARGET_ZYNQ_DEMO) $(TARGET_ZYNQMP_DEMO)
TARGETS += $(TARGET_ZYNQMP_LMAC2_DEMO)

all: $(TARGETS)

-include $(ZYNQ_OBJS:.o=.d)
-include $(ZYNQMP_OBJS:.o=.d)
-include $(ZYNQMP_LMAC2_OBJS:.o=.d)
CFLAGS += -MMD
CXXFLAGS += -MMD

ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
include $(VERILATOR_ROOT)/include/verilated.mk

#
# LMAC2
#
LM2_DIR=LMAC_CORE2/LMAC2_INFO/

V_LDLIBS += $(VOBJ_DIR)/Vlmac_wrapper_top__ALL.a
LM_CORE = lmac_wrapper_top.v
include files-lmac2.mk
$(VOBJ_DIR)/Vlmac_wrapper_top__ALL.a: $(LM_CORE)
	$(VENV) $(VERILATOR) $(VFLAGS) $^
	$(MAKE) -C $(VOBJ_DIR) -f Vlmac_wrapper_top.mk

$(ZYNQMP_LMAC2_TOP_O): $(V_LDLIBS)
$(ZYNQMP_TOP_O): $(V_LDLIBS)
$(VERILATED_O): $(V_LDLIBS)

$(VOBJ_DIR)/V%__ALL.a: %.v
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) -f V$(<:.v=.mk)
	$(MAKE) -C $(VOBJ_DIR) -f V$(<:.v=.mk) verilated.o

$(VOBJ_DIR)/Vaxilite_dev_v1_0__ALL.a: axilite_dev_v1_0.v axilite_dev_v1_0_S00_AXI.v
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) -f V$(<:.v=.mk)

$(VOBJ_DIR)/Vaxifull_dev_v1_0__ALL.a: axifull_dev_v1_0.v axifull_dev_v1_0_S00_AXI.v
	$(VENV) $(VERILATOR) $(VFLAGS) $<
	$(MAKE) -C $(VOBJ_DIR) -f V$(<:.v=.mk)
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

endif

$(TARGET_ZYNQ_DEMO): $(ZYNQ_OBJS) $(VTOP_LIB) $(VERILATED_O)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGETS)
	$(RM) $(ZYNQ_OBJS) $(ZYNQ_OBJS:.o=.d)
	$(RM) $(ZYNQMP_OBJS) $(ZYNQMP_OBJS:.o=.d)
	$(RM) $(ZYNQMP_LMAC2_OBJS) $(ZYNQMP_LMAC2_OBJS:.o=.d)
	$(RM) -r $(VOBJ_DIR) $(CSRC_DIR) *.daidir
