#include <tlsf.h>

void* __malloc_pool;

void* __malloc(size_t size, void* mem_pool) {
#ifdef LINUX
	return malloc(size);
#else
	if(!mem_pool)
		return malloc_ex(size, __malloc_pool);
	else 
		return malloc_ex(size, mem_pool);
#endif
}

void __free(void *ptr, void* mem_pool) { 
#ifdef LINUX
	return free(ptr);
#else
	if(!mem_pool) 
		free_ex(ptr, __malloc_pool);
	else
		free_ex(ptr, mem_pool);
#endif
}

void* __realloc(void *ptr, size_t new_size, void* mem_pool) {
#ifdef LINUX
	return realloc(ptr, new_size);
#else
	if(!mem_pool)
		return realloc_ex(ptr, new_size, __malloc_pool);
	else
		return realloc_ex(ptr, new_size, mem_pool);
#endif
}

void* __calloc(size_t nelem, size_t elem_size, void* mem_pool) {
#ifdef LINUX
	return calloc(nelem, elem_size);
#else
	if(!mem_pool) 
		return calloc_ex(nelem, elem_size, __malloc_pool);
	else
		return calloc_ex(nelem, elem_size, mem_pool);
#endif
}
