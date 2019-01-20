// Minimalistic Timer block. Part of C3P3u.
// Copyright (c) 2010 Edgar E. Iglesias
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
module apb_slave_timer(clk, rst,
			psel, penable, pwrite, paddr, pwdata, prdata,
			pready,
			irq);
	input	clk;
	input	rst;

	input	psel;
	input	penable;
	input	pwrite;
	input	[15:0] paddr;
	input	[31:0] pwdata;
	output	reg [31:0] prdata;
	output	reg pready;
	output	irq;

	// bit 0	enable cntdown
	// bit 1	irq pending
	reg	[1:0] tmr_cfg;
	reg	[31:0] tmr_cnt;
	reg	[31:0] tmr_div;
	reg	[31:0] tmr_free_cnt;

	assign irq = tmr_cfg[1];

always	@(posedge clk) begin
	tmr_free_cnt <= tmr_free_cnt + 1;

	if (tmr_cfg[0]) begin
		tmr_cnt <= tmr_cnt - 1;
		if (tmr_cnt == 0) begin
//			$display("timer hit, reloading %x\n", tmr_div);
			tmr_cnt <= tmr_div;
			tmr_cfg[1] <= 1;
		end
	end

	prdata <= 'bx;
	pready <= 0;
	if (psel) begin
		// We do single cycle accesses
		pready <= penable;
		if (penable && pwrite) begin
			case (paddr[15:2])
				0: begin
					tmr_cfg <= pwdata[1:0];
					// When enabling the timer, reload
					// the divisor.
					if (tmr_cfg[0] ^ pwdata[0]) begin
						if (pwdata[0]) begin
							tmr_cnt <= tmr_div;
						end
					end
				end

				1: tmr_cnt <= pwdata;
				2: begin tmr_div <= pwdata;
		//			$display("wirte %x to div", pwdata);
				end
				3: tmr_free_cnt <= pwdata;
			endcase
		end
		case (paddr[15:2])
			0: prdata[1:0] <= tmr_cfg;
			1: prdata <= tmr_cnt;
			2: prdata <= tmr_div;
			3: begin
				prdata <= tmr_free_cnt;
			end
		endcase
	end

	if (rst) begin
		tmr_free_cnt <= 0;
		tmr_cnt <= 0;
		tmr_div <= 0;
		tmr_cfg <= 0;
	end
end
endmodule
