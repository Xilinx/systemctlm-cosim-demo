//
// Copyright (C) 2018 Xilinx Inc.
// Written by Edgar E. Iglesias.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library release; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

module vlmac(
		clk,
		rst_n,

		xgmii_rxd,
		xgmii_rxc,

		xgmii_txd,
		xgmii_txc,

		tx_axis_mac_tdata,
		tx_axis_mac_tvalid,
		tx_axis_mac_tlast,
		tx_axis_mac_tuser,
		tx_axis_mac_tstrb,
		tx_axis_mac_tready,

		rx_axis_mac_tdata,
		rx_axis_mac_tvalid,
		rx_axis_mac_tlast,
		rx_axis_mac_tuser,
		rx_axis_mac_tstrb,
		rx_axis_mac_tready,

		host_addr_reg,
		reg_rd_start,
		reg_rd_done_out,
		FMAC_REGDOUT
	);

	input wire clk;
	input wire rst_n;

	input wire [63:0] xgmii_rxd;
	input wire [7:0] xgmii_rxc;
	output wire [63:0] xgmii_txd;
	output wire [7:0] xgmii_txc;

	input wire [63:0] tx_axis_mac_tdata;
	input wire tx_axis_mac_tvalid;
	input wire tx_axis_mac_tlast;
	input wire tx_axis_mac_tuser;
	input wire [7:0] tx_axis_mac_tstrb;
	output wire tx_axis_mac_tready;

	output wire [63:0] rx_axis_mac_tdata;
	output wire rx_axis_mac_tvalid;
	output wire rx_axis_mac_tlast;
	output wire rx_axis_mac_tuser;
	output wire [7:0] rx_axis_mac_tstrb;
	input wire rx_axis_mac_tready;

	input wire [15:0] host_addr_reg;
	input reg_rd_start;
	output reg_rd_done_out;
	output [31:0]     FMAC_REGDOUT;

	wire		linkup;
	AXIS_LMAC_TOP mac(
			.clk(clk),
			.xA_clk(clk),
			.reset_(rst_n),

			.gen_en_wr(1'b0),
			.fmac_speed(3'b0),

			.tx_mac_aclk(clk),
			.tx_axis_mac_tdata(tx_axis_mac_tdata),
			.tx_axis_mac_tvalid(tx_axis_mac_tvalid),
			.tx_axis_mac_tlast(tx_axis_mac_tlast),
			.tx_axis_mac_tuser(tx_axis_mac_tuser),
			.tx_axis_mac_tstrb(tx_axis_mac_tstrb),
			.tx_axis_mac_tready(tx_axis_mac_tready),

			.tx_ifg_delay(tx_ifg_delay),
			.tx_collision(tx_collision),
			.tx_retransmit(tx_retransmit),
			.tx_statistics_vector(tx_statistics_vector),
			.tx_statistics_valid(tx_statistics_valid),

			.rx_mac_aclk(clk),
			.rx_axis_mac_tdata(rx_axis_mac_tdata),
			.rx_axis_mac_tvalid(rx_axis_mac_tvalid),
			.rx_axis_mac_tlast(rx_axis_mac_tlast),
			.rx_axis_mac_tuser(rx_axis_mac_tuser),
			.rx_axis_mac_tstrb(rx_axis_mac_tstrb),
			.rx_axis_mac_tready(rx_axis_mac_tready),

			.rx_axis_filter_tuser(rx_axis_mac_filter_tuser),
			.rx_axis_compatible_mode(1'b1),
			.rx_statistics_vector(rx_statistics_vector),
			.rx_statistics_valid(rx_statistics_valid),

			.TCORE_MODE(1'b0),

			.xgmii_txd(xgmii_txd),
			.xgmii_txc(xgmii_txc),
			.xgmii_rxd(xgmii_rxd),
			.xgmii_rxc(xgmii_rxc),


			.host_addr_reg(host_addr_reg),
			.SYS_ADDR(4'b0),

			.fail_over(fail_over),
			.fmac_ctrl(32'h00000808),
			.fmac_ctrl1(32'h000005ee),
			.fmac_rxd_en(1'b1),

			.mac_pause_value(32'b0),
			.mac_addr0(48'h408c000001),

			.reg_rd_start(reg_rd_start),
			.reg_rd_done_out(reg_rd_done_out),
			.FMAC_REGDOUT(FMAC_REGDOUT)
			);
endmodule
