#include "asm.h"
#include "mp.h"

#include "gdt.h"

void gdt_init() {
	// GDTWR
	// 1MB ~ +264KB : Page table
	GDTR* gdtr = (GDTR*)GDTR_ADDR;
	GDTR_INIT(*gdtr);
	gdtr->limit = GDT_END_ADDR - GDT_ADDR - 1; 	// null, Kernel code segment, kernel data segment, task segment
	gdtr->base = GDT_ADDR;
	
	// Segments
	int i = 0;
	SD8* sd8 = (SD8*)GDT_ADDR;
	SD8_INIT(sd8[i]);		// null segment
	i++;
	
	SD8_INIT(sd8[i]);		// Kernel code segment
	SD8_BASE(sd8[i], 0);		// Ignored
	SD8_LIMIT(sd8[i], 0x0fffff);	// Ignored
	sd8[i].type = 0x0a;		// 1010 = Code, Execute, Read
	sd8[i].s = 1;
	sd8[i].dpl = 0;
	sd8[i].p = 1;
	sd8[i].g = 1;
	sd8[i].l = 1;
	i++;
	
	SD8_INIT(sd8[i]);		// Kernel data segment
	SD8_BASE(sd8[i], 0);		// Ignored
	SD8_LIMIT(sd8[i], 0x0fffff);	// Ignored
	sd8[i].type = 0x02;		// 0010 = Data, Read, Write
	sd8[i].s = 1;
	sd8[i].dpl = 0;
	sd8[i].p = 1;
	sd8[i].g = 1;
	sd8[i].l = 1;
	i++;
	
	SD8_INIT(sd8[i]);		// User data segment
	SD8_BASE(sd8[i], 0);		// Ignored
	SD8_LIMIT(sd8[i], 0x0fffff);	// Ignored
	sd8[i].type = 0x02;		// 0010 = Data, Read, Write
	sd8[i].s = 1;
	sd8[i].dpl = 3;
	sd8[i].p = 1;
	sd8[i].g = 1;
	sd8[i].l = 1;
	i++;
	
	SD8_INIT(sd8[i]);		// User code segment
	SD8_BASE(sd8[i], 0);		// Ignored
	SD8_LIMIT(sd8[i], 0x0fffff);	// Ignored
	sd8[i].type = 0x0a;		// 1010 = Code , Execute, Read
	sd8[i].s = 1;
	sd8[i].dpl = 3;
	sd8[i].p = 1;
	sd8[i].g = 1;
	sd8[i].l = 1;
	i++;
	
	SD16* sd16 = (SD16*)&sd8[i];
	for(int j = 0; j < MP_MAX_CORE_COUNT; j++) {	// 16 cores
		SD16_INIT(sd16[j]);			// TSS segment
		SD16_BASE(sd16[j], TSS_ADDR + j * sizeof(TSS));		// 1MB + sizeof(GDT)
		SD16_LIMIT(sd16[j], sizeof(TSS) - 1);	// 104 bytes
		sd16[j].type = 0x09;			// 1001 = Code, Execute-Only, accessed
		sd16[j].dpl = 0;
		sd16[j].p = 1;
		sd16[j].g = 1;
	}
}

void _dummy() {
}

void gdt_load() {
	GDTR* gdtr = (GDTR*)GDTR_ADDR;
	lgdt(gdtr);
}

void tss_init() {
	TSS* tss = (TSS*)TSS_ADDR;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		TSS_INIT(tss[i]);
		tss[i].ist[0] = PHYSICAL_TO_VIRTUAL(0x600000 - 0x50000); // Kernel interrupt stack
		tss[i].ist[1] = PHYSICAL_TO_VIRTUAL(0x600000 - 0x58000); // User interrupt stack
		tss[i].io_map = 0xffff;	// Not using I/O map
	}
}

void tss_load() {
	ltr(0x28 + mp_apic_id() * 16);		// TSS Segment offset
}
