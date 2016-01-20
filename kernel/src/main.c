// Standard C
#include <stdio.h>
#include <string.h>

// Core
#include <util/event.h>

// Kernel
#include "timer.h"
#include "malloc.h"
#include "gmalloc.h"
#include "page.h"
#include "stdio.h"
#include "port.h"
#include "mp.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "acpi.h"
#include "pci.h"
#include "apic.h"
#include "ioapic.h"
#include "task.h"
#include "icc.h"
#include "symbols.h"

// Disk
#include "driver/pata.h"
#include "driver/usb/usb.h"

// File system
#include "driver/fs.h"
#include "driver/bfs.h"

static void ap_timer_init() {
	extern const uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t tsc_ms;
	extern uint64_t tsc_us;
	extern uint64_t tsc_ns;

	*(uint64_t*)&TIMER_FREQUENCY_PER_SEC = *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&TIMER_FREQUENCY_PER_SEC);
	tsc_ms= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ms);
	tsc_us= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_us);
	tsc_ns= *(uint64_t*)VIRTUAL_TO_PHYSICAL((uint64_t)&tsc_ns);
}

void main(void) {
	mp_init();
	uint8_t apic_id = mp_apic_id();
	
	uint64_t vga_buffer = PHYSICAL_TO_VIRTUAL(0x600000 - 0x70000);
	stdio_init(apic_id, (void*)vga_buffer, 64 * 1024);
	malloc_init(vga_buffer);
	
	mp_sync();	// Barrier #1
	if(apic_id == 0) {
		// Parse kernel arguments
		uint32_t initrd_start = *(uint32_t*)(0x5c0000 - 0x400);
		uint32_t initrd_end = *(uint32_t*)(0x5c0000 - 0x400 + 8);
		
		printf("\x1b""32mOK""\x1b""0m\n");
		
		printf("Analyze CPU information...\n");
		cpu_init();
		gmalloc_init();
		timer_init(cpu_brand);
		
		gdt_init();
		tss_init();
		idt_init();
		
		mp_sync();	// Barrier #2
		
		printf("Initializing USB controller driver...\n");
		usb_initialize();
		
		printf("Initializing disk drivers...\n");
		disk_init();
		disk_register(&pata_driver, NULL);
		disk_register(&usb_msc_driver, NULL);
		
		printf("Initializing RAM disk...\n");
		memcpy((void*)0x400000 + 0x200000 * MP_MAX_CORE_COUNT, (void*)(uintptr_t)initrd_start, initrd_end - initrd_start);
		disk_register(&ramdisk_driver, "-addr 0x123456 -size 0x1234");
		
		printf("Initializing file system...\n");
		fs_init();
		fs_register(&bfs_driver);
		//fs_mount_root();
		
		printf("Loading GDT...\n");
		gdt_load();
		
		printf("Loading TSS...\n");
		tss_load();
		
		printf("Loading IDT...\n");
		idt_load();
		
		printf("Initializing ACPI...\n");
		acpi_init();
		
		printf("Initializing PCI...\n");
		pci_init();
	
		printf("Initailizing local APIC...\n");
		apic_init();
		
		printf("Initializing I/O APIC...\n");
		ioapic_init();
		apic_enable();
		
		printf("Initializing Multi-tasking...\n");
		task_init();
		
		printf("Initializing events...\n");
		event_init();
		
		printf("Initializing inter-core communications...\n");
		icc_init();
		
		printf("Initializing kernel symbols...\n");
		symbols_init();
		
	} else {
		mp_sync();	// Barrier #2
		ap_timer_init();
		
		gdt_load();
		tss_load();
		idt_load();
		
		apic_init();
		apic_enable();
		
		task_init();
		event_init();
		icc_init();
	}
	
	mp_sync();	// Barrier #3
	for(uint32_t i = 0; ; i++) {
		stdio_print_64(TIMER_FREQUENCY_PER_SEC, apic_id, 0);
		stdio_print_32(i, apic_id, 15);
		
		timer_swait(1);
	}
	
	while(1) asm("nop");
}
