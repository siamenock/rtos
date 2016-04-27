#include <tlsf.h>

extern void* __malloc_pool;

void* __malloc(size_t size, void* mem_pool) {
	if(!mem_pool)
		return malloc_ex(size, __malloc_pool);
	else 
		return malloc_ex(size, mem_pool);
}

void __free(void *ptr, void* mem_pool) { 
	if(!mem_pool) 
		free_ex(ptr, __malloc_pool);
	else
		free_ex(ptr, mem_pool);
}

void* __realloc(void *ptr, size_t new_size, void* mem_pool) {
	if(!mem_pool)
		return realloc_ex(ptr, new_size, __malloc_pool);
	else
		return realloc_ex(ptr, new_size, mem_pool);
}

void* __calloc(size_t nelem, size_t elem_size, void* mem_pool) {
	if(!mem_pool) 
		return calloc_ex(nelem, elem_size, __malloc_pool);
	else
		return calloc_ex(nelem, elem_size, mem_pool);
}
