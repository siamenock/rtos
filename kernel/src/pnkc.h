#ifndef __PNKC_H__
#define __PNKC_H__

#include <stdint.h>
#include <stdbool.h>

#define PNKC_TYPE_UNKNOWN	0
#define PNKC_TYPE_SYMBOL_MAP	1
#define PNKC_TYPE_MODULE	2

typedef struct {
	uint8_t		header[8];
	uint32_t	text_offset;
	uint32_t	text_size;
	uint32_t	rodata_offset;
	uint32_t	rodata_size;
	uint32_t	data_offset;
	uint32_t	data_size;
	uint32_t	bss_offset;
	uint32_t	bss_size;
	uint32_t	file_count;
	uint8_t		body[0];
} __attribute__((packed)) PNKC;

typedef struct {
	uint32_t	type;
	uint32_t	size;
	uint8_t*	address;
} __attribute__((packed)) PNKC_File;

PNKC* pnkc_find();
uint32_t pnkc_file_count();
bool pnkc_file_get(PNKC_File* file, uint32_t index);

#endif /* __PNKC_H__ */
