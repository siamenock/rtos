#include "stdio.h"
//#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <util/event.h>
#include <timer.h>
#include "mmap.h"

#include "apic.h"
#include "icc.h"
#include "mapping.h"
#include "shared.h"
#include "driver/console.h"
#include "gmalloc.h"
#include "vm.h"
#include "manager.h"
#include "shell.h"
#include "malloc.h"
#include "vnic.h"
#include "amp.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "smap.h"
#include "stdio.h"
#include "elf.h"
#include "device.h"
#include "popcorn.h"
#include "netlink.h"
#include "dispatcher.h"
#include "driver/nicdev.h"
#include "param.h"
#include "symbols.h"
#include "io_mux.h"

char* QWER;
static int kernel_symbols_init() {
	void* mmap_symbols[][2] = {
		{ &DESC_TABLE_AREA_START, "DESC_TABLE_AREA_START"},
		{ &DESC_TABLE_AREA_END, "DESC_TABLE_AREA_END"},

		{ &GDTR_ADDR, "GDTR_ADDR" },
		{ &GDT_ADDR, "GDT_ADDR" },
		{ &GDT_END_ADDR, "GDT_END_ADDR" },
		{ &TSS_ADDR, "TSS_ADDR" },

		{ &IDTR_ADDR, "IDTR_ADDR" },
		{ &IDT_ADDR, "IDT_ADDR" },
		{ &IDT_END_ADDR, "IDT_END_ADDR" },

		{ &KERNEL_TEXT_AREA_START, "KERNEL_TEXT_AREA_START" },
		{ &KERNEL_TEXT_AREA_END, "KERNEL_TEXT_AREA_END" },

		{ &KERNEL_DATA_AREA_START, "KERNEL_DATA_AREA_START" },
		{ &KERNEL_DATA_AREA_END, "KERNEL_DATA_AREA_END" },

		{ &KERNEL_DATA_START, "KERNEL_DATA_START" },
		{ &KERNEL_DATA_END, "KERNEL_DATA_END" },

		{ &VGA_BUFFER_START, "VGA_BUFFER_START" },
		{ &VGA_BUFFER_END, "VGA_BUFFER_END" },

		{ &USER_INTR_STACK_START, "USER_INTR_STACK_START" },
		{ &USER_INTR_STACK_END, "USER_INTR_STACK_END" },

		{ &KERNEL_INTR_STACK_START, "KERNEL_INTR_STACK_START" },
		{ &KERNEL_INTR_STACK_END, "KERNEL_INTR_STACK_END" },

		{ &KERNEL_STACK_START, "KERNEL_STACK_START" },
		{ &KERNEL_STACK_END, "KERNEL_STACK_END" },

		{ &PAGE_TABLE_START, "PAGE_TABLE_START" },
		{ &PAGE_TABLE_END, "PAGE_TABLE_END" },

		{ &LOCAL_MALLOC_START, "LOCAL_MALLOC_START" },
		{ &LOCAL_MALLOC_END, "LOCAL_MALLOC_END" },

		{ &SHARED_ADDR, "SHARED_ADDR" },
	};

	int count = sizeof(mmap_symbols) / (sizeof(char*) * 2);
	for(int i = 0; i < count; i++) {
		uint64_t symbol_addr = elf_get_symbol(mmap_symbols[i][1]);
		if(!symbol_addr) {
			printf("Can't Get Symbol Address: \"%s\"", mmap_symbols[i][1]);
			return -1;
		}
		char** test = (char**)mmap_symbols[i][0];

		if(!strcmp("SHARED_ADDR", mmap_symbols[i][1])) *test = (void*)VIRTUAL_TO_PHYSICAL(symbol_addr);
		else *test = (char*)symbol_addr;

		printf("\tSymbol \"%s\" : %p %p %p\n", mmap_symbols[i][1], symbol_addr, test, *test);
	}

	return 0;
}

static void _cpubrand(char* cpu_brand) {
	uint32_t* p = (uint32_t*)cpu_brand;
	uint32_t eax = 0x80000002;
	for(int i = 0; i < 12; i += 4) {
		asm volatile("cpuid"
			: "=a"(p[i + 0]), "=b"(p[i + 1]), "=c"(p[i + 2]), "=d"(p[i + 3])
			: "a"(eax++));
	}
}

// TODO: Fix Timer init libary
// How to get frequency per second in linux?
static void _timer_init(char* cpu_brand) {
	uint64_t _frequency;
	if(strstr(cpu_brand, "Intel") != NULL && strstr(cpu_brand, "@ ") != NULL) {
		int number = 0;
		int is_dot_found = 0;
		int dot = 0;

		const char* ch = strstr(cpu_brand, "@ ");
		ch += 2;
		while((*ch >= '0' && *ch <= '9') || *ch == '.') {
			if(*ch == '.') {
				is_dot_found = 1;
			} else {
				number *= 10;
				number += *ch - '0';

				dot += is_dot_found;
			}

			ch++;
		}

		uint64_t frequency = 0;
		if(strncmp(ch, "THz", 3) == 0) {
		} else if(strncmp(ch, "GHz", 3) == 0) {
			frequency = 1000000000L;
		} else if(strncmp(ch, "MHz", 3) == 0) {
			frequency = 1000000L;
		} else if(strncmp(ch, "KHz", 3) == 0) {
			frequency = 1000L;
		} else if(strncmp(ch, "Hz", 3) == 0) {
			frequency = 1L;
		}

		while(dot > 0) {
			frequency /= 10;
			dot--;
		}

		_frequency = frequency * number;
	} else {
		uint64_t time_tsc1 = timer_frequency();
		sleep(1);
		uint64_t time_tsc0 = timer_frequency();

		_frequency = time_tsc0 - time_tsc1;
	}

	//TODO timer init
	//How to initialize timer in linux application????
	extern uint64_t TIMER_FREQUENCY_PER_SEC;
	extern uint64_t __timer_ms;
	extern uint64_t __timer_us;
	extern uint64_t __timer_ns;

	printf("\tFreqeuncy : %lx\n", _frequency);
	unsigned long symbol_addr = elf_get_symbol("TIMER_FREQUENCY_PER_SEC");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"TIMER_FREQUENCY_PER_SEC\"");
	}
	uint64_t* _TIMER_FREQUENCY_PER_SEC = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*_TIMER_FREQUENCY_PER_SEC = _frequency;
	TIMER_FREQUENCY_PER_SEC = _frequency;

	symbol_addr = elf_get_symbol("__timer_ms");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_ms\"");
	}
	uint64_t* ___timer_ms = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	printf("\ttimer_ms symbol address: %p - %p\n", symbol_addr, ___timer_ms);
	*___timer_ms = _frequency / 1000;
	__timer_ms = _frequency / 1000;
	printf("\ttimer_ms: %x \n", *___timer_ms);

	symbol_addr = elf_get_symbol("__timer_us");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_us\"");
	}
	uint64_t* ___timer_us = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*___timer_us = *___timer_ms / 1000;
	__timer_us = __timer_ms / 1000;

	symbol_addr = elf_get_symbol("__timer_ns");
	if(!symbol_addr) {
		printf("Can't Get Symbol Address: \"__timer_ns\"");
	}
	uint64_t* ___timer_ns = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
	*___timer_ns = *___timer_us / 1000;
	__timer_ns = __timer_us / 1000;
}

static Device* devices[MAX_NIC_DEVICE_COUNT];
// static NICDevice* _nicdev_create(NICInfo* info) {
// 	extern uint64_t manager_mac;
// 	NICDevice* nic_device = gmalloc(sizeof(NICDevice));
// 	if(!nic_device)
// 		return NULL;
// 
// 	nic_device->mac = info->mac;
// 	strcpy(nic_device->name, info->name);
// 
// 	nicdev_register(nic_device);
// 
// 	printf("\tNIC Device created\n");
// 	printf("\t%s : [%02lx:%02lx:%02lx:%02lx:%02lx:%02lx] [%c]\n", nic_device->name,
// 			(info->mac >> 40) & 0xff,
// 			(info->mac >> 32) & 0xff,
// 			(info->mac >> 24) & 0xff,
// 			(info->mac >> 16) & 0xff,
// 			(info->mac >> 8) & 0xff,
// 			(info->mac >> 0) & 0xff,
// 			manager_mac == 0 ? '*' : ' ');
// 
// 	if(!manager_mac)
// 		manager_mac = info->mac;
// 
// 	return nic_device;
// }
// 
// int nicdev_init() {
// 	Device* dev;
// 	int index = 0;
// 	uint16_t count = device_count(DEVICE_TYPE_NIC);
// 	for(int i = 0; i < count; i++) {
// 		dev = device_get(DEVICE_TYPE_NIC, i);
// 		if(!dev)
// 			continue;
// 
// 		NICInfo info;
// 		NICDriver* driver = dev->driver;
// 		driver->get_info(dev->priv, &info);
// 
// 		if(info.name[0] == '\0')
// 			sprintf(info.name, "eth%d", index++);
// 
// 		NICDevice* nic_dev = _nicdev_create(&info);
// 		if(!nic_dev)
// 			return -1;
// 
// 		if(dispatcher_create_nicdev(nic_dev) < 0)
// 			return -2;
// 
// 		nic_dev->driver = driver;
// 		dev->priv = nic_dev;
// 	}
// 
// 	return 0;
// }

int main(int argc, char** argv) {
	int ret;

	//uint64_t vga_buffer = (uint64_t)VGA_BUFFER_START;
	char vga_buffer[64 * 1024];
	console_init();
	stdio_init(0, (void*)vga_buffer, 64 * 1024);

	if(geteuid() != 0) {
		printf("Permission denied. \n");
		printf("\tUsage: $sudo ./manager [BOOT PARAM FILE]\n");
		return -1;
	}

	if(argc != 2) {
		printf("\tUsage: $sudo ./manager [BOOT PARAM FILE]\n");
		return -1;
	}

	printf("\nPacketNgin 2.0 Manager\n");
	ret = smap_init();
	if(ret)
		goto error;

	printf("\nInitializing PacketNgin kernel module...\n");
	if(dispatcher_init() < 0)
		goto error;

	printf("\nParsing parameter...\n");
	ret = param_parse(argv[1]);
	if(ret) {
		printf("\tFailed to parse parameter\n");
		return ret;
	}

// 	printf("\nLoading %s...\n", kernel_elf);
// 	ret = elf_load(kernel_elf);
// 	if(ret)
// 		return ret;
	printf("\nInitializing memory mapping... \n");
	PHYSICAL_OFFSET = kernel_start_address - 0x400000;
	ret = mapping_memory();
	if(ret)
		return ret;

	printf("\nCopying kernel image...\n");
	ret = elf_copy(kernel_elf, kernel_start_address);
	if(ret) {
		printf("\tFailed to copy kernel image\n");
		return ret;
	}

	printf("\nInitiliazing symbols table...\n");
	symbols_init();

	printf("\nInitiliazing Kernel symbols...\n");
	ret = kernel_symbols_init();
	if(ret)
		return ret;

	printf("\nInitializing malloc area...\n");
	malloc_init();

	printf("\nInitializing timer...\n");

	char cpu_brand[4 * 4 * 3 + 1];
	_timer_init(cpu_brand);

	printf("\nInitializing shared memory area...\n");
	shared_init();

	printf("\nWake-Up Multi processor...\n");
	amp_init(kernel_start_address);
	mp_sync();	// Barrier #0
	/**
	 * Step 1. Kernel main
	 */
	printf("\nInitializing mulitiprocessing...\n");
	mp_init(); //Can't mp_init, Linux user application can't mmap bios memory space

	printf("\nInitializing cpu...\n");
	cpu_init();

	// TODO Fix Ramdisk Size
	printf("\nInitializing gmalloc area...\n");
	gmalloc_init();

	mp_sync(0); // Barrier #1

	printf("\nInitilizing GDT...\n");
	gdt_init();
	printf("\nInitilizing TSS...\n");
	tss_init();
	printf("\nInitilizing IDT...\n");
	idt_init();

	mp_sync(1); // Barrier #2

	printf("\nInitailizing local APIC...\n");
	if(mapping_apic() < 0)
		goto error;

	printf("\nInitializing events...\n");
	event_init();

	printf("\nInitializing I/O Multiplexer...\n");
	io_mux_init();

	printf("\nInitializing inter-core communications...\n");
	icc_init();

	printf("\nInitializing VM manager...\n");
	vm_init();

	printf("\nInitializing standard IO...\n");

	printf("\nInitializing linux netlink devices...\n");
	netlink_init();

// 	printf("\nInitializing NICs...\n");
// 	nicdev_init();

	printf("\nInitializing RPC manager...\n");
	manager_init();

	printf("\nInitializing shell...\n");
	shell_init();

	mp_sync(2);

	//TODO script
	bool script_process() {
// 		int fd = open("./boot.psh", O_RDONLY);
// 		if(fd == -1)
// 			return false;
// 
// 		command_process(fd);

		return true;
	}

 	script_process();

	while(1)
		event_loop();

	return 0;
error:
	printf("Manager initialization error occured. Terminated...\n");
	return -1;
}
