#include <limits.h>
#include <malloc.h>
#include <tlsf.h>

size_t __get_used_size(void *mem_pool) {

	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return INT_MAX;
#else
		extern void* __malloc_pool;
		return get_used_size(__malloc_pool);
#endif
	} else
		return get_used_size(mem_pool);
}

size_t __get_max_size(void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return INT_MAX;
#else
		extern void* __malloc_pool;
		return get_max_size(__malloc_pool);
#endif
	} else
		return get_max_size(mem_pool);
}

size_t __get_total_size(void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return INT_MAX;
#else
		extern void* __malloc_pool;
		return get_total_size(__malloc_pool);
#endif
	} else
		return get_total_size(mem_pool);
}

void* __malloc(size_t size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return malloc(size);
#else
		extern void* __malloc_pool;
		return malloc_ex(size, __malloc_pool);
#endif
	} else
		return malloc_ex(size, mem_pool);
}

void __free(void *ptr, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		free(ptr);
#else
		extern void* __malloc_pool;
		free_ex(ptr, __malloc_pool);
#endif
	} else
		free_ex(ptr, mem_pool);
}

void* __realloc(void *ptr, size_t new_size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return realloc(ptr, new_size);
#else
		extern void* __malloc_pool;
		return realloc_ex(ptr, new_size, __malloc_pool);
#endif
	} else
		return realloc_ex(ptr, new_size, mem_pool);
}

void* __calloc(size_t nelem, size_t elem_size, void* mem_pool) {
	if(!mem_pool) {
#ifdef LINUX /*LINUX*/
		return calloc(nelem, elem_size);
#else
		extern void* __malloc_pool;
		return calloc_ex(nelem, elem_size, __malloc_pool);
#endif
	} else
		return calloc_ex(nelem, elem_size, mem_pool);
}
