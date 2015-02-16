#include <stdint.h>
#include <thread.h>
#include <_malloc.h>
#include <gmalloc.h>

void* __gmalloc_pool;

void *gmalloc(size_t size) {
	return __malloc(size, __gmalloc_pool);
}

void gfree(void *ptr) {
	__free(ptr, __gmalloc_pool);
}

void *gcalloc(size_t nmemb, size_t size) {
	return __calloc(nmemb, size, __gmalloc_pool);
}

void *grealloc(void *ptr, size_t size) {
	return __realloc(ptr, size, __gmalloc_pool);
}
