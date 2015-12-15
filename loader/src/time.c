#include "time.h"

static uint32_t TIMER_FREQUENCY_PER_SEC;
static uint32_t cpu_ms;
static uint32_t cpu_us;
static uint32_t cpu_ns;

static uint64_t frequency() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));
	
	return time;
}

void time_init() {
#define PIT_CONTROL     0x43
#define PIT_COUNTER0    0x40
#define PIT_FREQUENCY	1193182
	uint8_t port_in8(uint16_t port) {
		uint8_t ret;
		asm volatile ("inb %%dx,%%al":"=a" (ret):"d" (port));
		return ret;
	}

	void port_out8(uint16_t port, uint8_t data) {
		asm volatile ("outb %%al,%%dx": :"d" (port), "a" (data));
	}

	uint16_t read_counter() {
		port_out8(PIT_CONTROL, 0x00 << 6 | 0x00 << 4 | 0x01 << 1 | 0x00 << 0);  // SC=Counter0, RW=Latch, Mode=1, BCD=Binary
		return (uint16_t)port_in8(PIT_COUNTER0) | ((uint16_t)port_in8(PIT_COUNTER0) << 8);
	}

	// Make PIT Counter stopped
	port_out8(PIT_CONTROL, 0x00 << 6 | 0x03 << 4 | 0x00 << 1 | 0x00 << 0);	// SC=Counter0, RW=0b11, Mode=0, BCD=Binary

	uint16_t freq = PIT_FREQUENCY * 50 / 1000;	// 50 ms
	port_out8(PIT_COUNTER0, freq & 0xff);

	uint64_t base = frequency();
	port_out8(PIT_COUNTER0, freq >> 8);

	// Wait using PIT controller
	port_out8(PIT_CONTROL, 0x00 << 6 | 0x03 << 4 | 0x01 << 1 | 0x00 << 0);	// SC=Counter0, RW=0b11, Mode=1, BCD=Binary

	uint16_t count = freq;
	while(!(count == 0 || count > freq)) {
		count = read_counter();
	}
	TIMER_FREQUENCY_PER_SEC = (frequency() - base) * 20;
	
	cpu_ms = TIMER_FREQUENCY_PER_SEC / 1000;
	cpu_us = cpu_ms / 1000;
	cpu_ns = cpu_us / 1000;
}

void time_swait(uint32_t s) {
	uint64_t time = frequency();
	time += TIMER_FREQUENCY_PER_SEC * s;
	while(frequency() < time)
		asm volatile("nop");
}

void time_mwait(uint32_t ms) {
	uint64_t time = frequency();
	time += cpu_ms * ms;
	while(frequency() < time)
		asm volatile("nop");
}

void time_uwait(uint32_t us) {
	uint64_t time = frequency();
	time += cpu_us * us;
	while(frequency() < time)
		asm volatile("nop");
}

void time_nwait(uint32_t ns) {
	uint64_t time = frequency();
	time += cpu_ns * ns;
	while(frequency() < time)
		asm volatile("nop");
}

