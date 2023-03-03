`default_nettype none


`include "FPGADesignElements/clog2_function.vh"

// https://en.wikipedia.org/wiki/Double-ended_queue

module deque
#(
	parameter WORD_WIDTH = 32,
	parameter MAX_SIZE   = 40, // total capacity of the queue
	`ifdef DEBUG
	parameter ADDR_INIT = 2,
	`endif
	parameter ADDR_WIDTH = clog2(MAX_SIZE)
)
(
	input wire clk,
	input wire rst,

	input wire                 front_push,
	input wire                 front_pop,
	input wire[WORD_WIDTH-1:0] front_push_data,
	output reg[WORD_WIDTH-1:0] front_pop_data,

	input wire                 back_push,
	input wire                 back_pop,
	input wire[WORD_WIDTH-1:0] back_push_data,
	output reg[WORD_WIDTH-1:0] back_pop_data,

	`ifdef DEBUG
	output wire[WORD_WIDTH-1:0] debug_buffer[MAX_SIZE-1:0],
	output wire[ADDR_WIDTH-1:0] debug_front_addr,
	output wire[ADDR_WIDTH-1:0] debug_back_addr,
	`endif

	output wire[ADDR_WIDTH:0] size, // count of pushes in the queue
	output wire empty,
	output reg full,

	// one element more conservative than above empty and full
	// just in case user wants to (push from both sides) or (pop from both sides)
	output wire                 low,
	output wire                 high
);

localparam ADDR_LAST = MAX_SIZE - 1;

`ifndef DEBUG
localparam ADDR_INIT = 0;
`endif
reg[WORD_WIDTH-1:0] buffer[MAX_SIZE-1:0];
reg[ADDR_WIDTH-1:0] front_addr;
reg[ADDR_WIDTH-1:0]  back_addr;

reg signed[2:0] delta;

function [ADDR_WIDTH-1:0] step (input [ADDR_WIDTH-1:0] addr, input signed[2:0] direction);
	begin
		step = signed'(ADDR_WIDTH+1)'(addr) + (ADDR_WIDTH+1)'(direction);
		if ((addr == 0) && (direction == -1))
			step = ADDR_LAST;
		if ((addr == ADDR_LAST) && (direction == +1))
			step = 0;
	end
endfunction

always @ (posedge clk) begin
	if (front_push) begin
		buffer[step(front_addr, -1)] <= front_push_data;
		front_addr <= step(front_addr, -1);
	end

	if (back_push) begin
		buffer[back_addr] <= back_push_data;
		back_addr <= step(back_addr, +1);
	end

	if (front_push || back_push) begin
		if (size + delta == MAX_SIZE)
			full <= 1;
	end

	if (front_pop) begin
		front_pop_data <= buffer[front_addr];
		`ifdef DEBUG buffer[front_addr] <= 0; `endif
		front_addr <= step(front_addr, +1);
	end

	if (back_pop) begin
		back_pop_data <= buffer[step(back_addr, -1)];
		`ifdef DEBUG buffer[step(back_addr, -1)] <= 0;`endif
		back_addr <= step(back_addr, -1);
	end

	if (front_pop || back_pop) begin
		full <= 0;
	end

	`ifdef DEBUG
	if (signed'(size) + signed'(delta) > MAX_SIZE) $error("Overflow");
	if (signed'(size) + signed'(delta) < 0)        $error("Underflow");  // BUG: at MAX_SIZE=4 we get constant underflows
	if (front_pop && front_push) $error("Can't both pop and push from the front");
	if (back_pop  && back_push)  $error("Can't both pop and push from the back");
	`endif

    if (rst) begin
		full <= 0;
		front_addr <= ADDR_INIT;
		back_addr  <= ADDR_INIT;

		`ifdef DEBUG
		buffer <= '{default: 0};

		front_pop_data <= 0;
		back_pop_data  <= 0;
		`endif
    end
end

always @ * begin
	size = (back_addr >= front_addr) ?
			back_addr - front_addr :
			MAX_SIZE + back_addr - front_addr;
	if (full) size = MAX_SIZE;
	empty = size == 0;

	delta = front_push + back_push - front_pop - back_pop;

	// conservative statuses for the user
	low = size <= 1;
	high = size >= ADDR_LAST;

	`ifdef DEBUG
	debug_buffer = buffer;
	debug_front_addr = front_addr;
	debug_back_addr = back_addr;
	`endif
end

endmodule
