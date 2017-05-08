#include <stdlib.h>

#include "pnkc.h"
#include "multiboot_tag.h"

void* multiboot_tag_get(uint32_t multiboot_tag_type) {
	PNKC* pnkc = (PNKC*)(0x200200 - sizeof(PNKC));
	uintptr_t addr = 0x200000 + (((uintptr_t)pnkc->smap_offset + (uintptr_t)pnkc->smap_size + 7) & ~7);
	uintptr_t end = addr + *(uint32_t*)addr;
	addr += 8;

	while(addr < end) {
		struct multiboot_tag* tag = (struct multiboot_tag*)addr;
		if(tag->type == multiboot_tag_type) {
			return tag;
		}

		addr = (addr + tag->size + 7) & ~7;
	}
	return NULL;
}
