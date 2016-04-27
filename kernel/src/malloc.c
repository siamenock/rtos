#include <tlsf.h>
#include <malloc.h>
#include <_malloc.h>
#include "stdio.h"
#include "pnkc.h"
#include "page.h"
#include "malloc.h"

#define DEBUG	0

#if DEBUG
#include <util/map.h>

typedef struct {
	uint64_t	count;
	uint64_t	size;
} Stat;

typedef struct {
	void*		caller;
	size_t		size;
} Trace;

static Map* statistics;
static Map* tracing;
static bool is_debug;
int debug_malloc_count;
int debug_free_count;
#endif /* DEBUG */

extern void* __malloc_pool;	// Defined in malloc.c from libcore

void malloc_init(uint64_t end) {
	PNKC* pnkc = (PNKC*)(0x200200 /* Kernel entry end */ - sizeof(PNKC));

	uint64_t addr1 = pnkc->data_offset + pnkc->data_size;
	uint64_t addr2 = pnkc->bss_offset + pnkc->bss_size;
	uint64_t start = PHYSICAL_TO_VIRTUAL(0x400000 + (addr1 > addr2 ? addr1 : addr2));
	
	__malloc_pool = (void*)start;
	init_memory_pool((uint32_t)(end - start), __malloc_pool, 0);

	#if DEBUG
	statistics = map_create(512, map_uint64_hash, map_uint64_equals, NULL);
	tracing = map_create(8192, map_uint64_hash, map_uint64_equals, NULL);
	is_debug = true;
	#endif /* DEBUG */
}

size_t malloc_total() {
	return get_total_size(__malloc_pool);
}

size_t malloc_used() {
	return get_used_size(__malloc_pool);
}

#if DEBUG
void malloc_statistics() {
	is_debug = false;
	
	printf("Caller                      Count             Size\n");
	MapIterator iter;
	map_iterator_init(&iter, statistics);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		Stat* s = entry->data;
		if(s->count >= 2 || s->size > 2048)
			printf("%p %16ld %16ld\n", entry->key, s->count, s->size);
	}
	
	printf("usage: %ld/%ld\n", malloc_used(), malloc_total());
	printf("tracing: class: %ld total: %ld\n", map_size(statistics), map_size(tracing));
	printf("malloc/free count: %d - %d = %d\n", debug_malloc_count, debug_free_count, debug_malloc_count - debug_free_count);
	debug_malloc_count = debug_free_count = 0;
	
	is_debug = true;
}

void malloced(void* caller, size_t size, void* ptr) {
	debug_malloc_count++;
	if(is_debug) {
		is_debug = false;
		
		if(!map_contains(statistics, caller)) {
			Stat* stat = malloc(sizeof(Stat));
			stat->count = 0;
			stat->size = 0;
			map_put(statistics, caller, stat);
		}
		
		Stat* stat = map_get(statistics, caller);
		stat->count++;
		stat->size += size;
		
		Trace* trace = malloc(sizeof(Trace));
		trace->caller = caller;
		trace->size = size;
		map_put(tracing, ptr, trace);
		
		is_debug = true;
	}
}

void freed(void* ptr) {
	debug_free_count++;
	if(is_debug) {
		is_debug = false;
		
		Trace* trace = map_remove(tracing, ptr);
		if(!trace) {
			printf("ERROR: freeing never alloced pointer: %p from: %p\n", ptr, ((void**)read_rbp())[1]);
			uint64_t* v = ptr;
			for(int i = 0; i < 16; i++) {
				printf("[%d] %p\n", i, v[i]);
			}
			while(1) asm("hlt");
		}
		
		Stat* stat = map_get(statistics, trace->caller);
		if(stat == NULL) {
			printf("ERROR: freeing not alloced pointer from: %p\n", ((void**)read_rbp())[1]);
			while(1) asm("hlt");
		}
		
		stat->count--;
		stat->size -= trace->size;
		
		if(stat->count == 0 && stat->size == 0) {
			map_remove(statistics, trace->caller);
			free(stat);
		} else if(stat->count == 0 && stat->size != 0) {
			printf("ERROR: malloc/free mismatching: count: %ld, size: %ld\n", stat->count, stat->size);
			while(1) asm("hlt");
		}
		
		free(trace);
		
		is_debug = true;
	}
}
#endif /* DEBUG */

#if 0
inline void* malloc(size_t size) {
	void* ptr = __malloc(size, __malloc_pool);
	
	if(!ptr) {
		// TODO: print to stderr
		printf("Not enough local memory!!!\n");
		printf("Requested size: %ld from %p\n", size, ((void**)read_rbp())[1]);
		
		#if DEBUG
		malloc_statistics();
		#endif /* DEBUG */
		
		while(1) asm("hlt");
	}
	
	#if DEBUG
	malloced(((void**)read_rbp())[1], size, ptr);
	#endif /* DEBUG */
	
	return ptr;
}

inline void free(void* ptr) {
	__free(ptr, __malloc_pool);
	
	#if DEBUG
	freed(ptr);
	#endif /* DEBUG */
}

inline void* realloc(void* ptr, size_t size) {
	void* ptr2 = __realloc(ptr, size, __malloc_pool);
	#if DEBUG
	freed(ptr);
	malloced(((void**)read_rbp())[1], size, ptr2);
	#endif /* DEBUG */
	return ptr2;
}

inline void* calloc(size_t nmemb, size_t size) {
	void* ptr = __calloc(nmemb, size, __malloc_pool);
	#if DEBUG
	malloced(((void**)read_rbp())[1], nmemb * size, ptr);
	#endif /* DEBUG */
	return ptr;
}
#endif
