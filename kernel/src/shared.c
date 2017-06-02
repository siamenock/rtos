#include <stdio.h>
#include <string.h>
#include "asm.h"
#include "shared.h"
#include "page.h"

void shared_init() {
	Shared* shared = (Shared*)SHARED_ADDR;
	uint8_t apic_id = mp_apic_id();
	if(apic_id == 0 /*BSP*/) {
		printf("\tShared space: %p\n", SHARED_ADDR);
	}

	if(shared->magic != SHARED_MAGIC) {
		printf("\tWrong Magic Number: %lx\n", shared->magic);
		hlt();
	}
}

void shared_sync() {
	Shared* shared = (Shared*)SHARED_ADDR;
	static uint8_t barrier;
	uint8_t apic_id = mp_apic_id();
	if(apic_id) {
		while(shared->sync <= barrier)
			asm volatile("nop");

		barrier++;
	} else {
		shared->sync = ++barrier;
	}
}
