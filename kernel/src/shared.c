#include <stdio.h>
#include <string.h>
#include "mmap.h"
#include "asm.h"
#include "shared.h"
#include "page.h"

int shared_init() {
	//Check Shared Area
	Shared* shared = (Shared*)SHARED_ADDR;
	if(shared->magic != SHARED_MAGIC) {
		return -1;
	}

	return 0;
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
