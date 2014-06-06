#include "page.h"

/**
 * TLB size: 256KB
 * TLB address: 6MB + core_id * 2MB - 256KB
 * TLB Blocks(4K blocks)
 * TLB[0] => l2
 * TLB[1] => l3u
 * TLB[2] => l3k
 * TLB[3~61] => l4u
 * TLB[62~63] => l4k
 */
int init_page_tables(uint8_t core_id) {
	uint32_t base = 0x600000 + core_id * 0x200000 - 0x40000;
	PageDirectory* l2 = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L2_INDEX);
	PageDirectory* l3u = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3U_INDEX);
	PageDirectory* l3k = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3K_INDEX);
	PageTable* l4u = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4U_INDEX);
	PageTable* l4k = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4K_INDEX);
	
	// Clean TLB area
	volatile uint32_t* p = (uint32_t*)base;
	for(int i = 0; i < 65536; i++)
		*p++ = 0;
	
	// Level 2
	l2[0].base = (uint32_t)l3u >> 12;
	l2[0].p = 1;
	l2[0].us = 1;
	l2[0].rw = 1;
	
	l2[511].base = (uint32_t)l3k >> 12;
	l2[511].p = 1;
	l2[511].us = 0;
	l2[511].rw = 1;
	
	// Level 3
	for(int i = 0; i < PAGE_L4U_SIZE; i++) {
		l3u[i].base = (uint32_t)&l4u[i * PAGE_ENTRY_COUNT] >> 12;
		l3u[i].p = 1;
		l3u[i].us = 1;
		l3u[i].rw = 1;
	}
	
	for(int i = 0; i < PAGE_L4K_SIZE; i++) {
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].base = (uint32_t)&l4k[i * PAGE_ENTRY_COUNT] >> 12;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].p = 1;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].us = 0;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].rw = 1;
	}
	
	// Level 4
	for(int i = 0; i < PAGE_L4U_SIZE * PAGE_ENTRY_COUNT; i++) {
		l4u[i].base = i;
		l4u[i].p = 1;
		l4u[i].us = 0;
		l4u[i].rw = 1;
		l4u[i].ps = 1;
	}
	
	// Kernel global area(gmalloc, segment descriptor, IDT, code, rodata)
	// Mapping 256MB to kernel
	for(int i = 0; i < 128; i++) {
		l4k[i].base = i;
		l4k[i].p = 1;
		l4k[i].us = 0;
		l4k[i].rw = 1;
		l4k[i].ps = 1;
	}
	
	// Kernel local area(malloc, TLB, TS, data, bss, stack)
	l4k[2 + core_id] = l4k[2];
	
	l4k[2].base = 2 + core_id;	// 2 * (2 + core_id)MB
	l4k[2].p = 1;
	l4k[2].us = 0;
	l4k[2].rw = 1;
	l4k[2].ps = 1;
	
	return 1;
}
