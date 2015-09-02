#include <string.h>
#include <stdint.h>
#include <time.h>

uint64_t cpu_ms;
uint64_t cpu_us;
uint64_t cpu_ns;
clock_t cpu_clock;
uint64_t cpu_frequency;

uint64_t cpu_tsc() {
	uint64_t time;
	uint32_t* p = (uint32_t*)&time;
	asm volatile("rdtsc" : "=a"(p[0]), "=d"(p[1]));
	
	return time;
}

void time_init() {
	// TODO: Measure more accurated value
	void measure() {
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

		// Wait using PIT controller
		port_out8(PIT_CONTROL, 0x00 << 6 | 0x03 << 4 | 0x01 << 1 | 0x00 << 0);	// SC=Counter0, RW=0b11, Mode=1, BCD=Binary
		uint16_t freq = PIT_FREQUENCY * 10 / 1000;	// 10 ms
		port_out8(PIT_COUNTER0, freq & 0xff);

		uint64_t base = cpu_tsc();
		port_out8(PIT_COUNTER0, freq >> 8);

		uint16_t count = freq;
		while(!(count == 0 || count > freq)) {
			count = read_counter();
		}
		cpu_frequency = (cpu_tsc() - base) * 100;
	}

	char cpu_brand[4 * 4 * 3 + 1];
	// Intel(R) Core(TM) i7-3770 CPU @ 3.40GHz
	if(strstr(cpu_brand, "Intel") != NULL && strstr(cpu_brand, "@ ") != NULL) {
		int number = 0;
		int is_dot_found = 0;
		int dot = 0;

		const char* ch = strstr(cpu_brand, "@ ");
		ch += 2;
		while((*ch >= '0' && *ch <= '9') || *ch == '.') {
			if(*ch == '.') {
				is_dot_found = 1;
			} else {
				number *= 10;
				number += *ch - '0';

				dot += is_dot_found;
			}

			ch++;
		}

		uint64_t frequency = 0;
		if(strncmp(ch, "THz", 3) == 0) {
		} else if(strncmp(ch, "GHz", 3) == 0) {
			frequency = 1000000000L;
		} else if(strncmp(ch, "MHz", 3) == 0) {
			frequency = 1000000L;
		} else if(strncmp(ch, "KHz", 3) == 0) {
			frequency = 1000L;
		} else if(strncmp(ch, "Hz", 3) == 0) {
			frequency = 1L;
		}

		while(dot > 0) {
			frequency /= 10;
			dot--;
		}

		cpu_frequency = frequency * number;
	} else {
		measure();
	}

	cpu_ms = cpu_frequency / 1000;
	cpu_us = cpu_ms / 1000;
	cpu_ns = cpu_us / 1000;

	cpu_clock = cpu_frequency / CLOCKS_PER_SEC;
}

void cpu_swait(uint32_t s) {
	uint64_t time = cpu_tsc();
	time += cpu_frequency * s;
	while(cpu_tsc() < time)
		asm volatile("nop");
}

void cpu_mwait(uint32_t ms) {
	uint64_t time = cpu_tsc();
	time += cpu_ms * ms;
	while(cpu_tsc() < time)
		asm volatile("nop");
}

void cpu_uwait(uint32_t us) {
	uint64_t time = cpu_tsc();
	time += cpu_us * us;
	while(cpu_tsc() < time)
		asm volatile("nop");
}

void cpu_nwait(uint32_t ns) {
	uint64_t time = cpu_tsc();
	time += cpu_ns * ns;
	while(cpu_tsc() < time)
		asm volatile("nop");
}

#include <stdio.h>

inline time_t time_ns() {
#ifdef LINUX
	struct timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return (clock_t)-1;

	return (time_t)(ts.tv_nsec + ts.tv_sec * 1000000000);

#else /*LINUX*/
	/* (Current CPU Time Stamp Counter / CPU frequency per nano-seconds) */
	return (time_t)(cpu_tsc() / cpu_ns); 
#endif
}

time_t time_us() {
	return time_ns() / 1000;
}

time_t time_ms() {
	return time_ns() / 1000000;
}

time_t time_s() {
	return time_ns() / 1000000000;
}

