#ifdef LINUX
#include <malloc.h>
#include <limits.h>

void* __malloc_pool = NULL;

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
#endif
