#include <stdio.h>
#include <tlsf.h>
#include "mmap.h"
#include "gmalloc.h"
#include "shared.h"

/*
 *#include <string.h>
 *#include <util/list.h>
 *#include <tlsf.h>
 *#include <malloc.h>
 *#include <_malloc.h>
 *#include "malloc.h"
 *#include "idt.h"
 *#include "pnkc.h"
 *#include "mp.h"
 *#include "shared.h"
 *#include "multiboot2.h"
 *#include "gmalloc.h"
 *#include "entry.h"
 *#include "mmap.h"
 */

uint32_t bmalloc_count;
uint64_t* bmalloc_pool;

void gmalloc_init() {
	/* Gmalloc pool area : IDT_END_ADDR */
	gmalloc_pool = (void*)VIRTUAL_TO_PHYSICAL(IDT_END_ADDR);

	bmalloc_pool = (void*)VIRTUAL_TO_PHYSICAL((uint64_t)shared->bmalloc_pool);
	bmalloc_count = shared->bmalloc_count;

	void print_pool(char* message, size_t total) {
		size_t mb = total / 1024 / 1024;
		size_t kb = total / 1024 - mb * 1024;
		printf("%s: %zd.%zdMB ", message, mb, kb);
	}
	
	printf("Memory pool: ");
	print_pool("global", gmalloc_total());
	print_pool("block", bmalloc_total());
	printf("\n");
}

inline size_t gmalloc_total() {
	return get_total_size(gmalloc_pool);
}

inline size_t gmalloc_used() {
	return get_used_size(gmalloc_pool);
}

inline void* gmalloc(size_t size) {
	do {
		void* ptr = malloc_ex(size, gmalloc_pool);
		if(ptr)
			return ptr;

		// TODO: print to stderr
		printf("WARN: Not enough global memory!!!\n");
		
		void* block = bmalloc();
		if(!block) {
			// TODO: print to stderr
			printf("ERROR: Not enough block memory!!!\n");
			return NULL;
		}
		
		add_new_area(block, 0x200000, gmalloc_pool);
	} while(1);
}

inline void gfree(void* ptr) {
	free_ex(ptr, gmalloc_pool);
}

inline void* grealloc(void* ptr, size_t size) {
	return realloc_ex(ptr, size, gmalloc_pool);
}

inline void* gcalloc(uint32_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, gmalloc_pool);
}

void* bmalloc() {
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(!(bmalloc_pool[i] & 0x01)) {
			void* ptr = (void*)bmalloc_pool[i];
			bmalloc_pool[i] |= 0x01;
			return ptr;
		}
	}
	
	// TODO: print to stderr
	printf("Not enough block memory!!!");
	
	return NULL;
}

void bfree(void* ptr) {
	uint64_t addr = (uint64_t)ptr;
	addr |= 0x01;
	
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] == addr) {
			bmalloc_pool[i] ^= 0x01;
			break;
		}
	}
}

size_t bmalloc_total() {
	return bmalloc_count * 0x200000;
}

size_t bmalloc_used() {
	size_t size = 0;
	for(size_t i = 0; i < bmalloc_count; i++) {
		if(bmalloc_pool[i] & 0x01) {
			size += 0x200000;
		}
	}
	
	return size;
}
