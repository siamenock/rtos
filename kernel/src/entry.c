#include <stdio.h>
#include "string.h"
#include "page.h"
#include "bootparam.h"
#include "entry.h"
#include "pnkc.h"
#include "mmap.h"

struct boot_params boot_params;
char boot_command_line[COMMAND_LINE_SIZE];
uint64_t PHYSICAL_OFFSET;
PNKC pnkc;

//#define __aligned(x)		__attribute__((aligned(x)))
//#define __always_unused		__attribute__((unused))

/*
 *static PageDirectory    _l2[512]	__aligned(4096) __always_unused;
 *static PageDirectory*   _l3u[512]	__aligned(4096) __always_unused;
 *static PageDirectory*   _l3k[512]	__aligned(4096) __always_unused;
 *static PageTable*       _l4u[512 * 59]  __aligned(4096) __always_unused;
 *static PageTable*       _l4k[512 * 2]   __aligned(4096) __always_unused;
 */

static __always_inline void init_page_tables(uint8_t apic_id, uint64_t offset) {
	//uint64_t virtual_base = (uint64_t)&_l2;
	uint64_t base = PAGE_TABLE_START + apic_id * 0x200000 + offset;
	//virtual_base - __START_KERNEL_map + offset; //0x600000 + apic_id * 0x200000 - 0x40000 + offset;
	PageDirectory* l2 = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L2_INDEX);
	PageDirectory* l3u = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3U_INDEX);
	PageDirectory* l3k = (PageDirectory*)(base + PAGE_TABLE_SIZE * PAGE_L3K_INDEX);
	PageTable* l4u = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4U_INDEX);
	PageTable* l4k = (PageTable*)(base + PAGE_TABLE_SIZE * PAGE_L4K_INDEX);

	// Clean TLB area*/
	volatile uint32_t* p = (uint32_t*)base;
	for(int i = 0; i < 65536; i++)
		*p++ = 0;

	// Level 2*/
	l2[0].base = (uint64_t)l3u >> 12;
	l2[0].p = 1;
	l2[0].us = 1;
	l2[0].rw = 1;

	l2[511].base = (uint64_t)l3k >> 12;
	l2[511].p = 1;
	l2[511].us = 0;
	l2[511].rw = 1;

	// Level 3
	for(int i = 0; i < PAGE_L4U_SIZE; i++) {
		l3u[i].base = (uint64_t)&l4u[i * PAGE_ENTRY_COUNT] >> 12;
		l3u[i].p = 1;
		l3u[i].us = 1;
		l3u[i].rw = 1;
	}

	for(int i = 0; i < PAGE_L4K_SIZE; i++) {
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].base = (uint64_t)&l4k[i * PAGE_ENTRY_COUNT] >> 12;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].p = 1;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].us = 0;
		l3k[PAGE_ENTRY_COUNT - PAGE_L4K_SIZE + i].rw = 1;
	}

	// Level 4
	for(int i = 0; i < PAGE_L4U_SIZE * PAGE_ENTRY_COUNT; i++) {
		l4u[i].base = i;// + (offset >> 21);
		l4u[i].p = 1;
		l4u[i].us = 1;
		l4u[i].rw = 1;
		l4u[i].ps = 1;
	}

	// Kernel global area(gmalloc, segment descriptor, IDT, code, rodata)
	// Mapping 256MB to kernel
	for(int i = 0; i < 128; i++) {
		l4k[i].base = i + (offset >> 21);
		l4k[i].p = 1;
		l4k[i].us = 0;
		l4k[i].rw = 1;
		l4k[i].ps = 1;
		l4k[i].exb = 0;
	}

	// Kernel global area(code, rodata, modules, gmalloc)
	l4k[1].exb = 0;

	// Kernel local area(malloc, TLB, TS, data, bss, stack)
	l4k[2 + apic_id] = l4k[2];

	l4k[2].base = 2 + apic_id + (offset >> 21);	// 2 * (2 + apic_id)MB
	l4k[2].p = 1;
	l4k[2].us = 0;
	l4k[2].rw = 1;
	l4k[2].ps = 1;
}

static __always_inline void activate_pml4(uint8_t apic_id, uint64_t offset) {
	//uint64_t virtual_base = (uint64_t)&_l2;
	uint64_t pml4 = PAGE_TABLE_START + apic_id * 0x200000 + offset;
		//virtual_base - __START_KERNEL_map + offset; //0x600000 + apic_id * 0x200000 - 0x40000 + offset;
	asm volatile("movq %0, %%cr3" : : "r"(pml4));
}

static __always_inline void activate_pae(uint8_t apic_id) {
#define PAE 0x620	// OSXMMEXCPT=1, OSFXSR=1, PAE=1

	uint64_t cr4;
	asm volatile("movq %%cr4, %0" : "=r"(cr4));

	cr4 |= PAE;
	asm volatile("movq %0, %%cr4" : : "r"(cr4));
}

static __always_inline void activate_longmode(uint8_t apic_id) {
#define LONGMODE	0x0901	// NXE=1, LME=1, SCE=1

	uint32_t msr;
	asm volatile("movl %1, %%ecx\n"
			"rdmsr\n"
			"movl %%eax, %0\n"
			: "=r"(msr)
			: "r"(0xc0000080));

	msr |= LONGMODE;

	asm volatile("movl %0, %%eax\n"
			"wrmsr"
			: : "r"(msr));
}

static __always_inline void activate_paging(uint8_t apic_id) {
#define ADD	0xe000003e	// PG=1, CD=1, NW=1, NE=1, TS=1, EM=1, MP=1
#define REMOVE	0x60000004	//       CD=1, NW=1,             EM=1

	uint64_t cr0;
	asm volatile("movq %%cr0, %0" : "=r"(cr0));
	cr0 |= ADD;
	cr0 ^= REMOVE;

	asm volatile("movq %0, %%cr0" : : "r"(cr0));

}

static __always_inline void save_bootdata(uint64_t** real_mode_data, uint64_t* offset) {
	asm volatile("movq %%rsi, %0" : "=m"(*real_mode_data));
	asm volatile("movq %%rdi, %0" : "=m"(*offset));
}

static __always_inline void init_kernel_stack() {
	uint64_t kernel_stack_end = KERNEL_STACK_END;
	asm volatile("movq %0, %%rsp" : : "m"(kernel_stack_end));
	asm volatile("movq %0, %%rbp" : : "m"(kernel_stack_end));
}

static void copy_bootdata(uint64_t* real_mode_data) {
	char* command_line;
	memcpy(&boot_params, real_mode_data, sizeof(boot_params));
	if (boot_params.hdr.cmd_line_ptr) {
		command_line = (char*)(uint64_t)boot_params.hdr.cmd_line_ptr;
		memcpy(boot_command_line, command_line, COMMAND_LINE_SIZE);
	}
}

static void reserve_initrd() {
	/* Assume only end is not page aligned */
	uint64_t ramdisk_shift = boot_params.hdr.ramdisk_shift;
	uint64_t ramdisk_image;
	uint64_t ramdisk_size  = boot_params.hdr.ramdisk_size;
	uint64_t ramdisk_end;

#define RAMDISK_MAGIC 0xdf
	if(boot_params.hdr.ramdisk_magic == RAMDISK_MAGIC)
		ramdisk_image = boot_params.hdr.ramdisk_image + (ramdisk_shift << 32);
	else
		ramdisk_image = boot_params.hdr.ramdisk_image;

#define PAGE_SIZE		4096
#define ALIGN(addr, align)	(((uintptr_t)(addr) + (align) - 1) & ~((align) - 1))
#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)

	ramdisk_end = PAGE_ALIGN(ramdisk_image + ramdisk_size);

	if(!boot_params.hdr.type_of_loader ||
			!ramdisk_image || !ramdisk_size)
		return;		/* No initrd provided by bootloader */

	pnkc.initrd_start = ramdisk_image;
	pnkc.initrd_end = ramdisk_end;
}

static void reserve_mmap() {
	pnkc.smap_count = boot_params.e820_entries;
	memcpy(pnkc.smap, boot_params.e820_map, sizeof(boot_params.e820_map));
}

/* C entry point of kernel */
void entry() {
	uint64_t* real_mode_data;
	uint64_t offset;
	save_bootdata(&real_mode_data, &offset);

	init_page_tables(0, offset);

	activate_pae(0);
	activate_pml4(0, offset);
	activate_longmode(0);
	activate_paging(0);

	copy_bootdata(real_mode_data);
	PHYSICAL_OFFSET = offset;

	init_kernel_stack();
	reserve_initrd();
	reserve_mmap();

	extern void main();
	main();
}
