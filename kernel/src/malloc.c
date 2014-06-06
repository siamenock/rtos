#include "../../loader/src/page.h"
#include <tlsf.h>
#include <malloc.h>
#include "stdio.h"
#include "pnkc.h"
#include "malloc.h"

//static void* pool;

void* __malloc_pool;

void malloc_init() {
	PNKC* pnkc = pnkc_find();
	uint64_t addr1 = pnkc->data_offset + pnkc->data_size;
	uint64_t addr2 = pnkc->bss_offset + pnkc->bss_size;
	uint64_t start = PHYSICAL_TO_VIRTUAL(0x400000 + (addr1 > addr2 ? addr1 : addr2));
	uint64_t end = PHYSICAL_TO_VIRTUAL(0x600000 - 0x60000);
	
	__malloc_pool = (void*)start;
	init_memory_pool((uint32_t)(end - start), __malloc_pool, 0);
}

uint64_t malloc_total() {
	return get_total_size(__malloc_pool);
}

uint64_t malloc_used() {
	return get_used_size(__malloc_pool);
}

inline void* malloc(size_t size) {
	void* ptr = malloc_ex(size, __malloc_pool);
	if(!ptr) {
		// TODO: print to stderr
		printf("Not enough local memory!!!");
	}
	return ptr;
}

inline void free(void* ptr) {
	free_ex(ptr, __malloc_pool);
}

inline void* realloc(void* ptr, size_t size) {
	return realloc_ex(ptr, size, __malloc_pool);
}

inline void* calloc(size_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, __malloc_pool);
}
