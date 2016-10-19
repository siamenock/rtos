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
//	printf("Core address : %p\n", shared->mp_cores);
	uint8_t* mp_cores = mp_core_map();

	/*
	 *printf("MP Cores : ");
	 *for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
	 *        printf("%02x ", mp_cores[i]);
	 *}
	 *printf("\n");
	 */

	memcpy(shared->mp_cores, mp_core_map(), sizeof(shared->mp_cores));

	// Bmalloc pool info
	shared->bmalloc_count = bmalloc_count;
	shared->bmalloc_pool = bmalloc_pool;

	//printf("Bmalloc : %p (%lx)\n", shared->bmalloc_pool, shared->bmalloc_count);
	
#define BUFFER_SIZE	4096

	extern char __stdin[BUFFER_SIZE];
	extern size_t __stdin_head;
	extern size_t __stdin_tail;
	extern size_t __stdin_size;

	extern char __stdout[BUFFER_SIZE];
	extern size_t __stdout_head;
	extern size_t __stdout_tail;
	extern size_t __stdout_size;

	extern char __stderr[BUFFER_SIZE];
	extern size_t __stderr_head;
	extern size_t __stderr_tail;
	extern size_t __stderr_size;

	// Standard I/O info
	/*
	 *printf("__Stdout : %p\n", __stdout);
	 *printf("__Stdout : %p\n", &__stdout);
	 */
	shared->__stdout = VIRTUAL_TO_PHYSICAL(&__stdout);
	shared->__stdout_head = VIRTUAL_TO_PHYSICAL(&__stdout_head);
	shared->__stdout_tail = VIRTUAL_TO_PHYSICAL(&__stdout_tail);
	shared->__stdout_size = VIRTUAL_TO_PHYSICAL(&__stdout_size);
/*
 *
 *        printf("Standard I/O: \n");
 *        printf("Stdout	: %p\n", shared->__stdout);
 *        printf("Haed	: %d\n", *shared->__stdout_head);
 *        printf("Tail	: %d\n", *shared->__stdout_tail);
 *        printf("Size	: %d\n", *shared->__stdout_size);
 */
}
