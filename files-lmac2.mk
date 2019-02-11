#
# systemctlm-cosim-demo Makefile fragment listing all LMAC2 verilog files.
#
# Copyright (c) 2019 Xilinx Inc.
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

LM_CORE += $(LM2_DIR)/AXIS_LMAC_TOP/AXIS_LMAC_TOP.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/AXIS_BRIDGE_TOP.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/axis2fib_txctrl.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/axis2fib_rxctrl.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/fib2fmac_txctrl.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/fmac2fib_rxctrl.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/txdata_fifo256x64.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/rxdata_fifo256x64.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/rxrbcnt_fifo4x32.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/txwbcnt_fifo4x32.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/LMAC_CORE_TOP.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/REG_IF_BRIDGE/RIF_IF_BRIDGE.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/REG_IF_BRIDGE/rxrregif_fifo4x32.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/REG_IF_BRIDGE/rxrregif_fifo4x8.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/REG_IF_BRIDGE/txwregif_fifo4x16.v
LM_CORE += $(LM2_DIR)/AXIS_BRIDGE/REG_IF_BRIDGE/txwregif_fifo4x8.v

LM_CORE += $(LM2_DIR)/ASYNCH_FIFO/asynch_fifo.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/br_sfifo4x32.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/bsh32_dn_88.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/bsh8_dn_64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/byte_reordering.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/crc32_d16s.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/crc32_d24s.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/crc32_d64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/crc32_d8s.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/ctrl_2G_5G.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/eth_crc32_gen.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/fmac_fifo4Kx64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/fmac_fifo4Kx8.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/fmac_fifo512x64_2clk.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/fmac_register_if_LE2.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/g2x_ctrl.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gige_crc32x64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gigerx_bcnt_fifo256x16.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gigerx_fifo256x64_2clk.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gigerx_fifo256x8.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gige_s2p.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gige_tx_encap.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/gige_tx_gmii.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/rx_5G.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/rx_decap_LE2.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/rx_xgmii_LE2.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tcore_fmac_core_LE2.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tx_10G_wrap.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tx_1G_wrap.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tx_encap.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/txfifo_1024x64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tx_mac10g_crc32x64.v
LM_CORE += $(LM2_DIR)/LMAC_CORE_TOP/tx_xgmii_LE2.v
