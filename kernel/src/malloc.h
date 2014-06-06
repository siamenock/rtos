#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <stdint.h>

void malloc_init();
uint64_t malloc_total();
uint64_t malloc_used();

/*
void* malloc(uint32_t size);
void free(void* ptr);
void* realloc(void* ptr, uint32_t size);
void* calloc(uint32_t nmemb, uint32_t size);
*/

#endif /* __MALLOC_H__ */
