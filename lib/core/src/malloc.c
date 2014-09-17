#include <malloc.h>

void* __malloc_pool;

#ifndef LINUX
#include <tlsf.h>

void* malloc(size_t size) {
	return malloc_ex(size, __malloc_pool);
}

void free(void* ptr) {
	free_ex(ptr, __malloc_pool);
}

void* realloc(void* ptr, size_t size) {
	return realloc_ex(ptr, size, __malloc_pool);
}

void* calloc(size_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, __malloc_pool);
}

#else /* LINUX */

#include <limits.h>

size_t get_used_size(void *mem_pool) {
	return INT_MAX;
}

size_t get_max_size(void* mem_pool) {
	return INT_MAX;
}

size_t get_total_size(void* mem_pool) {
	return INT_MAX;
}

void* malloc_ex(size_t size, void* mem_pool) {
	return malloc(size);
}

void free_ex(void *ptr, void* mem_pool) {
	free(ptr);
}

void* realloc_ex(void *ptr, size_t new_size, void* mem_pool) {
	return realloc(ptr, new_size);
}

void* calloc_ex(size_t nelem, size_t elem_size, void* mem_pool) {
	return calloc(nelem, elem_size);
}

#endif /* LINUX */
