#include <stdio.h>
#include "shared.h"
#include "mmap.h"

Shared* shared;

void shared_init() {
	/* Shared area : 2 MB - sizeof(Shared) */
	shared = (Shared*)(DESC_TABLE_AREA_END - sizeof(Shared));
	printf("Shared area : %lx \n", (uint64_t)shared);

	/*
	 *printf("Standard I/O: \n");
	 *printf("Stdout	: %p\n", shared->__stdout);
	 *printf("Haed	: %p\n", shared->__stdout_head);
	 *printf("Tail	: %p\n", shared->__stdout_tail);
	 *printf("Size	: %p\n", shared->__stdout_size);
	 */

	/*
	 *uint8_t* ptr = (uint8_t*)shared;
	 *for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
	 *        printf("%02x ", ptr[i]);
	 *}
	 *printf("\n");
	 */
}
