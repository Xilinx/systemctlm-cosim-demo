`timescale 1ps / 1fs
module test_bench();
    logic clk;
    logic rst;
`define DATA_WIDTH 32
`define ADDR_WIDTH 8
`define STRB_WIDTH 4
`define ID_WIDTH 8
`define PIPELINE_OUTPUT 0
  /*AUTOWIRE*/
  // Beginning of automatic wires (for undeclared instantiated-module outputs)
  wire                  s_axi_arready;          // From axi_ram_impl_inst of axi_ram_impl.v
  wire                  s_axi_awready;          // From axi_ram_impl_inst of axi_ram_impl.v
  wire [`ID_WIDTH-1:0]  s_axi_bid;              // From axi_ram_impl_inst of axi_ram_impl.v
  wire [1:0]            s_axi_bresp;            // From axi_ram_impl_inst of axi_ram_impl.v
  wire                  s_axi_bvalid;           // From axi_ram_impl_inst of axi_ram_impl.v
  wire [`DATA_WIDTH-1:0] s_axi_rdata;           // From axi_ram_impl_inst of axi_ram_impl.v
  wire [`ID_WIDTH-1:0]  s_axi_rid;              // From axi_ram_impl_inst of axi_ram_impl.v
  wire                  s_axi_rlast;            // From axi_ram_impl_inst of axi_ram_impl.v
  wire [1:0]            s_axi_rresp;            // From axi_ram_impl_inst of axi_ram_impl.v
  wire                  s_axi_rvalid;           // From axi_ram_impl_inst of axi_ram_impl.v
  wire                  s_axi_wready;           // From axi_ram_impl_inst of axi_ram_impl.v
  // End of automatics
  /*AUTOREG*/

    initial begin
        clk = 1'b0;
        forever begin
            #500 clk = ~clk;
        end
    end

    initial begin
        rst = 1'b1;
        #100000 rst = 1'b0;
    end

    axi_ram_impl axi_ram_impl_inst(/*AUTOINST*/
                                   // Outputs
                                   .s_axi_awready       (s_axi_awready),
                                   .s_axi_wready        (s_axi_wready),
                                   .s_axi_bid           (s_axi_bid),
                                   .s_axi_bresp         (s_axi_bresp),
                                   .s_axi_bvalid        (s_axi_bvalid),
                                   .s_axi_arready       (s_axi_arready),
                                   .s_axi_rid           (s_axi_rid),
                                   .s_axi_rdata         (s_axi_rdata),
                                   .s_axi_rresp         (s_axi_rresp),
                                   .s_axi_rlast         (s_axi_rlast),
                                   .s_axi_rvalid        (s_axi_rvalid),
                                   // Inputs
                                   .clk                 (clk),
                                   .rst                 (rst),
                                   .s_axi_awid          (s_axi_awid),
                                   .s_axi_awaddr        (s_axi_awaddr),
                                   .s_axi_awlen         (s_axi_awlen),
                                   .s_axi_awsize        (s_axi_awsize),
                                   .s_axi_awburst       (s_axi_awburst),
                                   .s_axi_awlock        (s_axi_awlock),
                                   .s_axi_awcache       (s_axi_awcache),
                                   .s_axi_awprot        (s_axi_awprot),
                                   .s_axi_awvalid       (s_axi_awvalid),
                                   .s_axi_wdata         (s_axi_wdata),
                                   .s_axi_wstrb         (s_axi_wstrb),
                                   .s_axi_wlast         (s_axi_wlast),
                                   .s_axi_wvalid        (s_axi_wvalid),
                                   .s_axi_bready        (s_axi_bready),
                                   .s_axi_arid          (s_axi_arid),
                                   .s_axi_araddr        (s_axi_araddr),
                                   .s_axi_arlen         (s_axi_arlen),
                                   .s_axi_arsize        (s_axi_arsize),
                                   .s_axi_arburst       (s_axi_arburst),
                                   .s_axi_arlock        (s_axi_arlock),
                                   .s_axi_arcache       (s_axi_arcache),
                                   .s_axi_arprot        (s_axi_arprot),
                                   .s_axi_arvalid       (s_axi_arvalid),
                                   .s_axi_rready        (s_axi_rready)
    );


    initial begin                                                                                                                                                                               
        $fsdbDumpfile("tb_cohort.fsdb",50);                                                                                                                                                     
        $fsdbDumpvars(0, test_bench,"+all");                                                                                                                                                     
        $fsdbDumpvars(0, sc_main,"+all");                                                                                                                                                     
    end

endmodule


// Local Variables:
// verilog-library-files: ("axi_ram_impl.sv")
// End:
