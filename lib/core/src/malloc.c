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

#endif /* LINUX */
