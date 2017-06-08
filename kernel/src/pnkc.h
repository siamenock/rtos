#ifndef __PNKC_H__
#define __PNKC_H__

#include <stdint.h>
#include "smap.h"

#define PNKC_MAGIC	0x020a0d0f434b4e50

typedef struct {
	uint64_t	magic;
	uint32_t	text_offset;
	uint32_t	text_size;
	uint32_t	rodata_offset;
	uint32_t	rodata_size;

	uint32_t	smap_offset;
	uint32_t	smap_size;

	uint32_t	data_offset;
	uint32_t	data_size;
	uint32_t	bss_offset;
	uint32_t	bss_size;
	uint32_t	initrd_start;
	uint32_t	initrd_end;
	uint8_t		smap_count;
	SMAP		smap[SMAP_MAX];
} __attribute__((packed)) PNKC;

extern PNKC pnkc;

#endif /* __PNKC_H__ */
