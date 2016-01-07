#include <stdio.h>

#include "timer.h"

#include "malloc.h"
#include "gmalloc.h"
#include "page.h"
#include "stdio.h"
#include "port.h"
#include "mp.h"
#include "cpu.h"

static void bp_timer_init() {
	// e.g. Intel(R) Core(TM) i7-3770 CPU @ 3.40GHz
	if(strstr(cpu_brand, "Intel") == NULL || strstr(cpu_brand, "@ ") == NULL)
		return;
	
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
	
	extern const uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t tsc_ms;
	extern uint64_t tsc_us;
	extern uint64_t tsc_ns;
	
	*(uint64_t*)&TIMER_FREQUENCY_PER_SEC = frequency * number;
	tsc_ms= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ms);
	tsc_us= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_us);
	tsc_ns= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ns);
}

static void ap_timer_init() {
	extern const uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t tsc_ms;
	extern uint64_t tsc_us;
	extern uint64_t tsc_ns;

	*(uint64_t*)&TIMER_FREQUENCY_PER_SEC = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&TIMER_FREQUENCY_PER_SEC);
	tsc_ms= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ms);
	tsc_us= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_us);
	tsc_ns= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ns);
}

void main(void) {
	mp_init();
	uint8_t apic_id = mp_apic_id();
	
	uint64_t vga_buffer = PHYSICAL_TO_VIRTUAL(0x600000 - 0x70000);
	stdio_init(apic_id, (void*)vga_buffer, 64 * 1024);
	malloc_init(vga_buffer);
	mp_sync();
	if(apic_id == 0) {
		printf("\x1b""32mOK""\x1b""0m\n");
		
		printf("Analyze CPU information...\n");
		cpu_init();
		bp_timer_init();
		
		mp_sync();
	} else {
		mp_sync();
		ap_timer_init();
	}
	
	mp_sync();
	for(uint32_t i = 0; ; i++) {
		stdio_print_64(TIMER_FREQUENCY_PER_SEC, apic_id, 0);
		stdio_print_32(i, apic_id, 15);
		
		timer_swait(1);
	}
	
	while(1) asm("nop");
}
