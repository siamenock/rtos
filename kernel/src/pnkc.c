#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "pnkc.h"

#define START	0x10000		// 512 bytes aligned
#define SIZE	(512 * 20)

PNKC* pnkc_find() {
	uint8_t* addr = (void*)START;
	uint8_t* end = addr + SIZE;
	while(addr < end) {
		if(addr[0] == 0x0f && addr[1] == 0x0d && addr[2] == 0x0a && addr[3] == 0x02 && addr[4] == 'P' && addr[5] == 'N' && addr[6] == 'K' && addr[7] == 'C')
			return (PNKC*)addr;
		
		addr += 512;
	}
	
	return NULL;
}

uint32_t pnkc_file_count() {
	PNKC* pnkc = pnkc_find();
	if(!pnkc) {
		printf("\tError: PNKC header not found: %p ~ %p\n", START, START + SIZE);
		return 0;
	}
	
	return pnkc->file_count;
}

bool pnkc_file_get(PNKC_File* file, uint32_t index) {
	PNKC* pnkc = pnkc_find();
	if(!pnkc) {
		printf("\tError: PNKC header not found: %p ~ %p\n", START, START + SIZE);
		return false;
	}
	
	if(index >= pnkc->file_count) {
		return false;
	}
	
	PNKC_File* f = (PNKC_File*)pnkc->body;
	uint8_t* body = pnkc->body + pnkc->text_size + pnkc->rodata_size + pnkc->data_size + 8 * pnkc->file_count;
	for(int i = 0; i < index; i++) {
		body += f->size;
		f = (void*)f + 8;	// uint32_t type, uint32_t size
	}
	
	file->type = f->type;
	file->size = f->size;
	file->address = body;
	
	return true;
}
