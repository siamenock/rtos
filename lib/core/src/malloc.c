#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <_malloc.h>

/* These functions could be replaced with other C library implemtation with strong symbols */
void* __malloc_pool;

void* __attribute__((weak)) malloc(size_t size) {
	return __malloc(size, __malloc_pool);
}

void __attribute__((weak)) free(void *ptr) { 
	__free(ptr, __malloc_pool);
}

void* __attribute__((weak)) realloc(void *ptr, size_t new_size) {
	return __realloc(ptr, new_size, __malloc_pool);
}

void* __attribute__((weak)) calloc(size_t nelem, size_t elem_size) {
	return __calloc(nelem, elem_size, __malloc_pool);
}

