#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "mmap.h"

extern uint32_t bmalloc_count;
extern uint64_t* bmalloc_pool;

Shared* shared = (Shared*)(PHYSICAL_TO_VIRTUAL(KERNEL_TEXT_AREA_START) - sizeof(Shared));

void shared_init() {
	memset(shared, 0x0, sizeof(Shared));

	// Core mapping info
	printf("Core address : %p\n", shared->mp_cores);
	uint8_t* mp_cores = mp_core_map();

	printf("MP Cores : ");
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		printf("%02x ", mp_cores[i]);
	}
	printf("\n");

	memcpy(shared->mp_cores, mp_core_map(), sizeof(shared->mp_cores));

	// Bmalloc pool info
	shared->bmalloc_count = bmalloc_count;
	shared->bmalloc_pool = bmalloc_pool;

	printf("Bmalloc : %p (%lx)\n", shared->bmalloc_pool, shared->bmalloc_count);
}
