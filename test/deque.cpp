#include <stdio.h>
#include <stdlib.h>
#include "verilated.h"
#include "Vdeque.h"

void clock(Vdeque *tb) {
	tb->eval();
	tb->clk = 0;
	tb->eval();
	tb->clk = 1;
	tb->eval();
}

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
void deque_print(Vdeque *tb) {
	#define WHITE "\033[37;1m"
	#define GRAY "\033[30;1m"
	printf(GRAY " ");
	const int max_size = COUNT_OF(tb->debug_buffer);
	if (tb->debug_front_addr > tb->debug_back_addr || tb->full) printf(WHITE);
	for(int i = 0; i < max_size; i++) {
		if (!tb->full) {
			if (i == tb->debug_front_addr) printf(WHITE);
			if (i == tb->debug_back_addr) printf(GRAY);
		} else
			if (i == tb->debug_back_addr) printf("\b|");
		printf("%2d ", tb->debug_buffer[i]);
	}
	printf("\033[30;1m // \033[34m%2d/%d\033[33m", tb->size, max_size);

	if (tb->low) printf(" low");
	if (tb->empty) printf(" empty");
	if (tb->high) printf(" HIGH");
	if (tb->full) printf(" FULL!");

	printf("\033[0m"); // reset
	printf("\x1B[s\n");
}

unsigned int deque_test_zero_count(Vdeque *tb) {
	const int max_size = COUNT_OF(tb->debug_buffer);
	unsigned int count = 0;
	for(int i = 0; i < max_size; i++)
		if (tb->debug_buffer[i] == 0)
			count++;
	return count;
}

void deque_front_push(Vdeque *tb, int front_push_data) {
	tb->front_push = 1;
	tb->front_push_data = front_push_data;
	clock(tb);
	tb->front_push = 0;
}

void deque_back_push(Vdeque *tb, int back_push_data) {
	tb->back_push = 1;
	tb->back_push_data = back_push_data;
	clock(tb);
	tb->back_push = 0;
}

void deque_push_both(Vdeque *tb, int front_push_data, int back_push_data) {
	tb->front_push = 1;
	tb->back_push = 1;
	tb->front_push_data = front_push_data;
	tb->back_push_data = back_push_data;
	clock(tb);
	tb->front_push = 0;
	tb->back_push = 0;
}

void deque_front_pop(Vdeque *tb, int *front_pop_data) {
	tb->front_pop = 1;
	clock(tb);
	tb->front_pop = 0;
	*front_pop_data = tb->front_pop_data;
}

void deque_back_pop(Vdeque *tb, int *back_pop_data) {
	tb->back_pop = 1;
	clock(tb);
	tb->back_pop = 0;
	*back_pop_data = tb->back_pop_data;
}

void deque_pop_both(Vdeque *tb, int *front_pop_data, int *back_pop_data) {
	tb->front_pop = 1;
	tb->back_pop = 1;
	clock(tb);
	tb->front_pop = 0;
	tb->back_pop = 0;
	*front_pop_data = tb->front_pop_data;
	*back_pop_data = tb->back_pop_data;
}

void deque_test_rst(Vdeque *tb) {
	tb->rst = 1;
	clock(tb);
	tb->rst = 0;
	deque_print(tb);
}

void deque_test_fill(Vdeque *tb) {
	// Initialize and fill deque with 0 for future tests
	const int max_size = COUNT_OF(tb->debug_buffer);
	deque_test_rst(tb);
	for(int i = 0; i < max_size*2; i++) {
		if(tb->full) break;
		deque_back_push(tb, 10+i);
	}
	deque_print(tb);
}

int main(int argc, char **argv) {
	Verilated::commandArgs(argc, argv);
	Vdeque *tb = new Vdeque;
	const int max_size = COUNT_OF(tb->debug_buffer);

	tb->front_push = 0;
	tb->front_pop = 0;
	tb->back_push = 0;
	tb->back_pop = 0;

	printf("\nback_push:\n");
	deque_test_rst(tb);
	for(int i = 0; i < max_size*2; i++) {
		if(tb->full) break;
		deque_back_push(tb, 10+i);
		deque_print(tb);
	}
	assert(deque_test_zero_count(tb) == 0);

	printf("\nfront_push:\n");
	deque_test_rst(tb);
	for(int i = 0; i < max_size*2; i++) {
		if(tb->full) break;
		deque_front_push(tb, (max_size+10-1)-i);
		deque_print(tb);
	}
	assert(deque_test_zero_count(tb) == 0);

	printf("\fpush both even:\n");
	deque_test_rst(tb);
	for(int i = 0; i < max_size*2; i++) {
		if(tb->high) break;
		deque_push_both(tb, 10+i, (max_size+10-1)-i);
		deque_print(tb);
	}
	assert(deque_test_zero_count(tb) <= 1);

	printf("\fpush both odd:\n");
	deque_test_rst(tb);
	deque_back_push(tb, 99);
	deque_print(tb);
	for(int i = 0; i < max_size*2; i++) {
		if(tb->high) break;
		deque_push_both(tb, 10+i, (max_size+10-2)-i);
		deque_print(tb);
	}
	assert(deque_test_zero_count(tb) <= 1);

	printf("\ndeque_test_fill:\n");
	deque_test_fill(tb);
	assert(deque_test_zero_count(tb) == 0);
	int v, v2;
	#define print_v()    printf("\x1B[u\x1B[1A \033[32m>> \033[92m%d\033[0m\n", v);
	#define print_v_v2() printf("\x1B[u\x1B[1A \033[32m>> \033[92m%2d %2d\033[0m\n", v, v2);

	printf("\nfront_pop:\n");
	deque_test_fill(tb);
	v = 0;
	for(int i = 0; i < max_size*2; i++) {
		if(tb->empty) break;
		deque_front_pop(tb, &v);
		deque_print(tb); print_v();
	}
	assert(deque_test_zero_count(tb) == max_size);

	printf("\nback_pop:\n");
	deque_test_fill(tb);
	v = 0;
	for(int i = 0; i < max_size*2; i++) {
		if(tb->empty) break;
		deque_back_pop(tb, &v);
		deque_print(tb); print_v();
	}
	assert(deque_test_zero_count(tb) == max_size);

	printf("\n pop both even:\n");
	deque_test_fill(tb);
	v = 0;
	v2 = 0;
	for(int i = 0; i < max_size*2; i++) {
		if(tb->low) break;
		deque_pop_both(tb, &v, &v2);
		deque_print(tb); print_v_v2();
	}
	assert(deque_test_zero_count(tb) >= max_size - 1);

	printf("\n pop both odd:\n");
	deque_test_fill(tb);
	deque_front_pop(tb, &v);
	deque_print(tb); print_v();
	v = 0;
	v2 = 0;
	for(int i = 0; i < max_size*2; i++) {
		if(tb->low) break;
		deque_pop_both(tb, &v, &v2);
		deque_print(tb); print_v_v2();
	}
	assert(deque_test_zero_count(tb) >= max_size - 1);

	return 0;
}
