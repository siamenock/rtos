#ifndef __GMALLOC_H__
#define __GMALLOC_H__

#include <stdint.h>
#include <stddef.h>

void gmalloc_init();
void gmalloc_extend();
size_t gmalloc_total();
size_t gmalloc_used();

void* gmalloc(size_t size);
void gfree(void* ptr);
void* grealloc(void* ptr, size_t size);
void* gcalloc(uint32_t nmemb, size_t size);

void* bmalloc();
void bfree(void* ptr);
size_t bmalloc_total();
size_t bmalloc_used();

#endif /* __GMALLOC_H__ */
