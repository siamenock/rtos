#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "page.h"

void shared_init() {
	Shared* shared = SHARED_ADDR;
	uint8_t apic_id = mp_apic_id();
	if(!apic_id) {
		printf("\tShared space: %p\n", SHARED_ADDR);
	}

	if(shared->magic != SHARED_MAGIC) {
		printf("\tWrong Magic Number: %lx\n", shared->magic);
		while(1);
	}
}
