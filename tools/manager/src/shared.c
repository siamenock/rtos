#include <stdio.h>
#include <string.h>
#include "mp.h"
#include "shared.h"
#include "mmap.h"
#include "page.h"

Shared* shared;

void shared_init() {
	shared = (Shared*)(VIRTUAL_TO_PHYSICAL((uint64_t)DESC_TABLE_AREA_END) - sizeof(Shared));
	printf("\tShared space: %p\n", shared);
	if(!mp_apic_id()) {
		memset(shared, 0x0, sizeof(Shared));
		shared->magic = SHARED_MAGIC;
		memset((void *)shared->mp_cores, MP_CORE_INVALID, MP_MAX_CORE_COUNT * sizeof(uint8_t));
	} else if(shared->magic != SHARED_MAGIC) {
		printf("ERROR: Wrong Shared Magic Number\n");
	}

	printf("\tShared magic: %p\n", shared->magic);
}
