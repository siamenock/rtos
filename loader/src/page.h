#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdint.h>

#define PAGE_ENTRY_COUNT	512
#define PAGE_TABLE_SIZE		0x1000		// 4096
#define PAGE_PAGE_SIZE		0x200000	// 2M

#define PAGE_L2_INDEX		0
#define PAGE_L3U_INDEX		1
#define PAGE_L3K_INDEX		2
#define PAGE_L4U_INDEX		3
#define PAGE_L4U_SIZE		59
#define PAGE_L4K_INDEX		62
#define PAGE_L4K_SIZE		2
#define PAGE_L4U	((PageTable*)(PHYSICAL_TO_VIRTUAL(0x600000 - 0x40000) + PAGE_TABLE_SIZE * PAGE_L4U_INDEX))
#define PAGE_L4U_BASE(coreid)	((PageTable*)(0x600000L + (coreid) * 0x200000 - 0x40000 + PAGE_TABLE_SIZE * PAGE_L4U_INDEX))

#define VIRTUAL_TO_PHYSICAL(addr)	(~0xffffffff80000000L & (addr))
#define PHYSICAL_TO_VIRTUAL(addr)	(0xffffffff80000000L | (addr))

#define TRANSLATE_TO_PHYSICAL(vaddr)	((uint64_t)PAGE_L4U[vaddr >> 21].base << 21 | (vaddr & 0x1fffff))

#define TRANSLATE_TO_PHYSICAL_BASE(vaddr, coreid)	((uint64_t)PAGE_L4U_BASE((coreid))[vaddr >> 21].base << 21 | (vaddr & 0x1fffff))

/* Page Map Level 4 Table Entry, Page Directory Pointer Table Entry */
typedef struct {
	uint64_t p: 1;		// Present
	uint64_t rw: 1;		// Read/Write
	uint64_t us: 1;		// User/Supevisor
	uint64_t pwt: 1;	// Page Level Write-through
	uint64_t pcd: 1;	// Page Level Cache Disable
	uint64_t a: 1;		// Accessed
	uint64_t reserved_lower: 3;
	uint64_t avail_lower: 3;
	uint64_t base: 28;
	uint64_t reserved_upper: 12;
	uint64_t avail_upper: 11;
	uint64_t exb: 1;	// Execute Disable
} __attribute__ ((packed)) PageDirectory;

/* Page Directory Pointer Table Entry */
typedef struct {
	uint64_t p: 1;		// Present
	uint64_t rw: 1;		// Read/Write
	uint64_t us: 1;		// User/Supervisor
	uint64_t pwt: 1;	// Page Level Write-through
	uint64_t pcd: 1;	// Page Cache Disable
	uint64_t a: 1;		// Accessed
	uint64_t d: 1;		// Dirty
	uint64_t ps: 1;		// Page Size
	uint64_t g: 1;		// Global
	uint64_t avail_lower: 3;
	uint64_t pat: 1;	// Page Attribute Table Index
	uint64_t reserved_lower: 8;
	uint64_t base: 19;
	uint64_t reserved_upper: 12;
	uint64_t avail_upper: 11;
	uint64_t exb: 1;	// Execute Disable
} __attribute__ ((packed)) PageTable;

int init_page_tables(uint8_t core_id);

#endif /* __PAGE_H__ */
