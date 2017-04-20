#ifndef __SMAP_H__
#define __SMAP_H__
#include <stdint.h>

#define SMAP_TYPE_MEMORY        1
#define SMAP_TYPE_RESERVED      2
#define SMAP_TYPE_ACPI          3
#define SMAP_TYPE_NVS           4
#define SMAP_TYPE_UNUSABLE      5
#define SMAP_TYPE_DISABLED      6

#define SMAP_EXT_NON_VOLATILE	1
#define SMAP_EXT_SLOW_ACCESS	2
#define SMAP_EXT_ERROR_LOG	    3

#define SMAP_MAX                128

typedef struct {
	uint64_t	base;
	uint64_t	length;
	uint32_t	type;
	//uint32_t	extended;
} __attribute__((packed)) SMAP;

uint8_t     smap_count;
SMAP        smap[SMAP_MAX];

int smap_init();
void smap_dump();
int smap_update_memmap(char* str);
int smap_update_mem(char* str);

#endif /* __SMAP_H__ */
