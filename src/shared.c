#include <stdio.h>
#include "shared.h"
#include "mmap.h"

Shared* shared;

void shared_init() {
	/* Shared area : 2 MB - sizeof(Shared) */
	shared = (Shared*)(DESC_TABLE_AREA_END - sizeof(Shared));
	printf("Shared area : %lx \n", (uint64_t)shared);

	uint8_t* ptr = (uint8_t*)shared;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		printf("%02x ", ptr[i]);
	}
	printf("\n");
}
