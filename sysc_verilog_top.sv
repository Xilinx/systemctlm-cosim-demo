`timescale 1ps / 1fs

`define U_DUT_TOP test_bench.axi_ram_impl_inst
`define DATA_WIDTH 32
`define ADDR_WIDTH 8
`define STRB_WIDTH 4
`define ID_WIDTH 8
`define PIPELINE_OUTPUT 0
/*
* AXI4 RAM
*/
module sysc_verilog_top
    (
        input clk,
        input                   rst,
        input                   rst_n,

        input  wire [`ID_WIDTH-1:0]    s_axi_awid,
        input  wire [`ADDR_WIDTH-1:0]  s_axi_awaddr,
        input  wire [7:0]             s_axi_awlen,
        input  wire [2:0]             s_axi_awsize,
        input  wire [1:0]             s_axi_awburst,
        input  wire                   s_axi_awlock,
        input  wire [3:0]             s_axi_awcache,
        input  wire [2:0]             s_axi_awprot,
        input  wire                   s_axi_awvalid,
        output wire                   s_axi_awready,
        input  wire [`DATA_WIDTH-1:0]  s_axi_wdata,
        input  wire [`STRB_WIDTH-1:0]  s_axi_wstrb,
        input  wire                   s_axi_wlast,
        input  wire                   s_axi_wvalid,
        output wire                   s_axi_wready,
        output wire [`ID_WIDTH-1:0]    s_axi_bid,
        output wire [1:0]             s_axi_bresp,
        output wire                   s_axi_bvalid,
        input  wire                   s_axi_bready,
        input  wire [`ID_WIDTH-1:0]    s_axi_arid,
        input  wire [`ADDR_WIDTH-1:0]  s_axi_araddr,
        input  wire [7:0]             s_axi_arlen,
        input  wire [2:0]             s_axi_arsize,
        input  wire [1:0]             s_axi_arburst,
        input  wire                   s_axi_arlock,
        input  wire [3:0]             s_axi_arcache,
        input  wire [2:0]             s_axi_arprot,
        input  wire                   s_axi_arvalid,
        output wire                   s_axi_arready,
        output wire [`ID_WIDTH-1:0]    s_axi_rid,
        output wire [`DATA_WIDTH-1:0]  s_axi_rdata,
        output wire [1:0]             s_axi_rresp,
        output wire                   s_axi_rlast,
        output wire                   s_axi_rvalid,
        input  wire                   s_axi_rready
    );

    //TODO: convert to svt axi interface

    // Don't touch clocks, clocks are managed within soc
    //    force axi_ram_impl.clk =     clk;
    //    force axi_ram_impl.rst =                                       rst;

    always @(*) begin
        force `U_DUT_TOP.s_axi_awid =                                s_axi_awid;
        force `U_DUT_TOP.s_axi_awaddr=                              s_axi_awaddr;
        force `U_DUT_TOP.s_axi_awlen=                               s_axi_awlen;
        force `U_DUT_TOP.s_axi_awsize=                              s_axi_awsize;
        force `U_DUT_TOP.s_axi_awburst=                            s_axi_awburst;
        force `U_DUT_TOP.s_axi_awlock=                              s_axi_awlock;
        force `U_DUT_TOP.s_axi_awcache=                            s_axi_awcache;
        force `U_DUT_TOP.s_axi_awprot=                             s_axi_awprot;
        force `U_DUT_TOP.s_axi_awvalid=                             s_axi_awvalid;

        force  s_axi_awready  = `U_DUT_TOP.s_axi_awready;
        force  s_axi_wready   = `U_DUT_TOP.s_axi_wready;
        force  s_axi_bid      = `U_DUT_TOP.s_axi_bid;
        force  s_axi_bresp    = `U_DUT_TOP.s_axi_bresp;
        force  s_axi_bvalid   = `U_DUT_TOP.s_axi_bvalid;
        force  s_axi_arready  = `U_DUT_TOP.s_axi_arready;
        force  s_axi_rid      = `U_DUT_TOP.s_axi_rid;
        force  s_axi_rdata    = `U_DUT_TOP.s_axi_rdata;
        force  s_axi_rresp    = `U_DUT_TOP.s_axi_rresp;
        force  s_axi_rlast    = `U_DUT_TOP.s_axi_rlast;
        force  s_axi_rvalid   = `U_DUT_TOP.s_axi_rvalid;


        force `U_DUT_TOP.s_axi_wdata   = s_axi_wdata;
        force `U_DUT_TOP.s_axi_wstrb   = s_axi_wstrb;
        force `U_DUT_TOP.s_axi_wlast   = s_axi_wlast;
        force `U_DUT_TOP.s_axi_wvalid  = s_axi_wvalid;
        force `U_DUT_TOP.s_axi_bready  = s_axi_bready;
        force `U_DUT_TOP.s_axi_arid    = s_axi_arid;
        force `U_DUT_TOP.s_axi_araddr  = s_axi_araddr;
        force `U_DUT_TOP.s_axi_arlen   = s_axi_arlen;
        force `U_DUT_TOP.s_axi_arsize  = s_axi_arsize;
        force `U_DUT_TOP.s_axi_arburst = s_axi_arburst;
        force `U_DUT_TOP.s_axi_arlock  = s_axi_arlock;
        force `U_DUT_TOP.s_axi_arcache = s_axi_arcache;
        force `U_DUT_TOP.s_axi_arprot  = s_axi_arprot;
        force `U_DUT_TOP.s_axi_arvalid = s_axi_arvalid;
        force `U_DUT_TOP.s_axi_rready  = s_axi_rready;
    end
endmodule
