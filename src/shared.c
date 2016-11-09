#include <stdio.h>
#include <string.h>
#include "mp.h"
#include "shared.h"
#include "mmap.h"

Shared* shared;

int shared_init() {
	shared = (Shared*)(KERNEL_TEXT_AREA_START - sizeof(Shared));
	printf("Shared space: %p\n", shared);
	if(!mp_apic_id()) {
		memset(shared, 0x0, sizeof(Shared));
		shared->magic = SHARED_MAGIC;
		memset(shared->mp_cores, MP_CORE_INVALID, MP_MAX_CORE_COUNT * sizeof(uint8_t));
	} else if(shared->magic != SHARED_MAGIC) {
		printf("Wrong Shared Magic Number\n");
		return -1;
	}

	return 0;
}
