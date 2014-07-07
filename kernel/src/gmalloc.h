#ifndef __GMALLOC_H__
#define __GMALLOC_H__

#include <stdint.h>
#include <stddef.h>

#define SMAP_TYPE_MEMORY	1
#define SMAP_TYPE_RESERVED	2
#define SMAP_TYPE_ACPI		3
#define SMAP_TYPE_NVS		4
#define SMAP_TYPE_UNUSABLE	5
#define SMAP_TYPE_DISABLED	6

#define SMAP_EXT_NON_VOLATILE	1
#define SMAP_EXT_SLOW_ACCESS	2
#define SMAP_EXT_ERROR_LOG	3

typedef struct {
	uint64_t	base;
	uint64_t	length;
	uint32_t	type;
	uint32_t	extended;
} __attribute__((packed)) SMAP;


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
