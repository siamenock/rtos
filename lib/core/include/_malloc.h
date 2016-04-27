/* Wrapper of TLSF malloc */
#ifndef ___MALLOC_H__
#define ___MALLOC_H__

#include <stddef.h>

void* __malloc(size_t size, void* mem_pool);
void __free(void *ptr, void* mem_pool);
void* __realloc(void *ptr, size_t new_size, void* mem_pool);
void* __calloc(size_t nelem, size_t elem_size, void* mem_pool);

#endif /* ___MALLOC_H__ */
