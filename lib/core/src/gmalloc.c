#include <stdint.h>
#include <thread.h>
#include <tlsf.h>
#include <gmalloc.h>

void* __gmalloc_pool;

void *gmalloc(size_t size) {
	return malloc_ex(size, __gmalloc_pool);
}

void gfree(void *ptr) {
	free_ex(ptr, __gmalloc_pool);
}

void *gcalloc(size_t nmemb, size_t size) {
	return calloc_ex(nmemb, size, __gmalloc_pool);
}

void *grealloc(void *ptr, size_t size) {
	return realloc_ex(ptr, size, __gmalloc_pool);
}
