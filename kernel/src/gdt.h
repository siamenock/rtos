#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>
#include "page.h"
#include "mp.h"

#define GDTR_ADDR	PHYSICAL_TO_VIRTUAL(0x100000)
#define GDTR_END_ADDR	(GDTR_ADDR + 16)
#define GDT_ADDR	GDTR_END_ADDR
#define GDT_END_ADDR	(GDT_ADDR + 8 * 5 + 16 * MP_MAX_CORE_COUNT)	// SD8 * 5 + SD16 * 16
#define TSS_ADDR	GDT_END_ADDR
#define TSS_END_ADDR	(TSS_ADDR + 104 * MP_MAX_CORE_COUNT)

typedef struct {
	uint16_t limit;
	uint64_t base;
	uint16_t padding;
	uint32_t padding2;
} __attribute__ ((packed)) GDTR;

typedef struct {
	uint64_t limit: 16;	// Segment limit
	uint64_t base: 24;	// Segment base address
	uint64_t type: 4;	// Segment type
	uint64_t s: 1;		// Descriptor type (0 = system, 1 = code or data)
	uint64_t dpl: 2;	// Descriptor privilege level
	uint64_t p: 1;		// Segment present
	uint64_t limit2: 4;
	uint64_t avl: 1;	// Available for use by system software
	uint64_t l: 1;		// 64bit code segment
	uint64_t db: 1;		// Default operation size/default stack pointer size and/or upper bound
	uint64_t g: 1;		// Granularity
	uint64_t base2: 8;
} __attribute__ ((packed)) SD8;

typedef struct {
	uint64_t limit: 16;	// Segment limit
	uint64_t base: 24;	// Segment base address
	uint64_t type: 4;	// Segment type
	uint64_t s: 1;		// Descriptor type (0 = system, 1 = code or data)
	uint64_t dpl: 2;	// Descriptor privilege level
	uint64_t p: 1;		// Segment present
	uint64_t limit2: 4;
	uint64_t avl: 1;	// Available for use by system software
	uint64_t l: 1;		// 64bit code segment
	uint64_t db: 1;		// Default operation size/default stack pointer size and/or upper bound
	uint64_t g: 1;		// Granularity
	uint64_t base2: 40;
	uint64_t reserved: 32;
} __attribute__ ((packed)) SD16;


typedef struct {
	uint32_t reserved;
	uint64_t rsp[3];
	uint64_t reserved2;
	uint64_t ist[7];
	uint64_t reserved3;
	uint16_t reserved4;
	uint16_t io_map;		// I/O Map Base Address
} __attribute__ ((packed)) TSS;	// Task Status Segment

#define GDTR_INIT(gdtr)		*(uint64_t*)&(gdtr) = 0; \
				*((uint64_t*)&(gdtr) + 1) = 0;

#define SD8_INIT(sd)		*(uint64_t*)&(sd) = 0;

#define SD8_BASE(sd, b)		(sd).base = (b) & 0xffffff; \
				(sd).base2 = ((b) >> 24) & 0xff;

#define SD8_LIMIT(sd, l)	(sd).limit = (l) & 0xffff; \
				(sd).limit2 = ((l) >> 16) & 0x0f;

#define SD16_INIT(sd)		*(uint64_t*)&(sd) = 0; \
				*((uint64_t*)&(sd) + 1) = 0;

#define SD16_BASE(sd, b)	(sd).base = (b) & 0xffffff; \
				(sd).base2 = ((b) >> 24) & 0xffffffffff;

#define SD16_LIMIT(sd, l)	(sd).limit = (l) & 0xffff; \
				(sd).limit2 = ((l) >> 16) & 0x0f;

#define TSS_INIT(tss)		*((uint64_t*)&(tss) + 0) = 0; \
				*((uint64_t*)&(tss) + 1) = 0; \
				*((uint64_t*)&(tss) + 2) = 0; \
				*((uint64_t*)&(tss) + 3) = 0; \
				*((uint64_t*)&(tss) + 4) = 0; \
				*((uint64_t*)&(tss) + 5) = 0; \
				*((uint64_t*)&(tss) + 6) = 0; \
				*((uint64_t*)&(tss) + 7) = 0; \
				*((uint64_t*)&(tss) + 8) = 0; \
				*((uint64_t*)&(tss) + 9) = 0; \
				*((uint64_t*)&(tss) + 10) = 0; \
				*((uint64_t*)&(tss) + 11) = 0; \
				*((uint64_t*)&(tss) + 12) = 0;

void gdt_init();
void gdt_load();

void tss_init();
void tss_load();

#endif /* __GDT_H__ */
