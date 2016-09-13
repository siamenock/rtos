#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <tlsf.h>

extern void* __malloc_pool;	// Defined in malloc.c from libcore

#define LOCAL_MALLOC_SIZE	0x1000000 /* 16 MB */
void malloc_init() {
	__malloc_pool = malloc(LOCAL_MALLOC_SIZE);
	if(!__malloc_pool) {
		printf("Malloc pool init failed. Terminated...\n");
		exit(-1);
	}

	printf("malloc_pool : %p\n", __malloc_pool);
	init_memory_pool(LOCAL_MALLOC_SIZE, __malloc_pool, 0);
}

size_t malloc_total() {
	return 0;
}

size_t malloc_used() {
	return 0;
}

