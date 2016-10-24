#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "mmap.h"

extern uint32_t bmalloc_count;
extern uint64_t* bmalloc_pool;

Shared* shared = (Shared*)(PHYSICAL_TO_VIRTUAL(KERNEL_TEXT_AREA_START) - sizeof(Shared));

void shared_init() {
	memset(shared, 0x0, sizeof(Shared));

	// Magic number info
	shared->magic = SHARED_MAGIC;

	// Core mapping info
	memcpy(shared->mp_cores, mp_core_map(), sizeof(shared->mp_cores));

	// Bmalloc pool info
	shared->bmalloc_count = bmalloc_count;
	shared->bmalloc_pool = bmalloc_pool;

}
