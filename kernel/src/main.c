#include <stdio.h>
#include <string.h>

#include "timer.h"

#include "malloc.h"
#include "gmalloc.h"
#include "page.h"
#include "stdio.h"
#include "port.h"
#include "mp.h"
#include "cpu.h"

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
		printf("\x1b""32mOK""\x1b""0m\n");
		
		printf("Analyze CPU information...\n");
		cpu_init();
		gmalloc_init();
		timer_init(cpu_brand);
		
		mp_sync();	// Barrier #2
		
		printf("Initializing USB controller driver...\n");
		usb_initialize();
		
		printf("Initializing disk drivers...\n");
		disk_init();
		disk_register(&pata_driver);
		disk_register(&usb_msc_driver);
		
		printf("Initializing file system...\n");
		bfs_init();
		fs_init();
	} else {
		mp_sync();	// Barrier #2
		ap_timer_init();
	}
	
	mp_sync();
	for(uint32_t i = 0; ; i++) {
		stdio_print_64(TIMER_FREQUENCY_PER_SEC, apic_id, 0);
		stdio_print_32(i, apic_id, 15);
		
		timer_swait(1);
	}
	
	while(1) asm("nop");
}
