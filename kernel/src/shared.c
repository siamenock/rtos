#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "mmap.h"

extern uint64_t* gmalloc_pool;
extern uint32_t bmalloc_count;
extern uint64_t* bmalloc_pool;

Shared* shared;

void shared_init() {
	shared = (Shared*)(DESC_TABLE_AREA_END - sizeof(Shared));

/*
 *        // Core mapping info
 *        memcpy(shared->mp_cores, mp_core_map(), sizeof(shared->mp_cores));
 *        
 *        // Gmalloc pool info
 *        shared->gmalloc_pool = gmalloc_pool;
 *        printf("Gmalloc Pool : %p\n", gmalloc_pool);
 *
 *        // Bmalloc pool info
 *        shared->bmalloc_count = bmalloc_count;
 *        shared->bmalloc_pool = bmalloc_pool;
 *
 *        printf("Bmalloc Pool : %p\n", bmalloc_pool);
 *        // Magic number info
 *        shared->magic = SHARED_MAGIC;
 */
}
