#include <stdio.h>
#include <stdlib.h>
#define __USE_POSIX
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <util/event.h>
#include <timer.h>
#include "apic.h"
#include "icc.h"
#include "mapping.h"
#include "shared.h"
#include "gmalloc.h"
#include "vm.h"
#include "manager.h"
#include "shell.h"
#include "malloc.h"
#include "vnic.h"
#include "cpu.h"
#include "gdt.h"
#include "idt.h"
#include "socket.h"
#include "driver/nic.h"
#include "smap.h"
#include "elf.h"
#include "popcorn.h"

static bool idle0_event() {
	extern Device* nic_current;
	int poll_count = 0;
	for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
		Device* dev = nic_devices[i];
		if(dev == NULL)
			break;

		nic_current = dev;
		NICDriver* nic = nic_current->driver;

		poll_count += nic->poll(nic_current->id);
	}
	return true;
}

static void init_nics(int count) {
	int index = 0;
	extern uint64_t manager_mac;
	for(int i = 0; i < count; i++) {
		nic_devices[i] = device_get(DEVICE_TYPE_NIC, i);
		if(!nic_devices[i])
			continue;

		NICPriv* nic_priv = gmalloc(sizeof(NICPriv));
		if(!nic_priv)
			continue;

		nic_priv->nics = map_create(8, NULL, NULL, gmalloc_pool);

		nic_devices[i]->priv = nic_priv;

		NICInfo info;
		((NICDriver*)nic_devices[i]->driver)->get_info(nic_devices[i]->id, &info);

		nic_priv->port_count = info.port_count;
		for(int j = 0; j < info.port_count; j++) {
			nic_priv->mac[j] = info.mac[j];

			char name_buf[64];
			sprintf(name_buf, "eth%d", index);
			uint16_t port = j << 12;

			Map* vnics = map_create(16, NULL, NULL, gmalloc_pool);
			map_put(nic_priv->nics, (void*)(uint64_t)port, vnics);
			
			printf("NICs in physical NIC(%s): %p\n", name_buf, nic_priv->nics);
			printf("\t%s : [%02lx:%02lx:%02lx:%02lx:%02lx:%02lx] [%c]\n", name_buf,
					(info.mac[j] >> 40) & 0xff,
					(info.mac[j] >> 32) & 0xff,
					(info.mac[j] >> 24) & 0xff,
					(info.mac[j] >> 16) & 0xff,
					(info.mac[j] >> 8) & 0xff,
					(info.mac[j] >> 0) & 0xff,
					manager_mac == 0 ? '*' : ' ');

			if(!manager_mac)
				manager_mac = info.mac[j];

			index++;
		}
	}
}

extern int cpu_start;
extern int cpu_end;
static int parse_present_mask(char* present_mask) {
	char* next = present_mask;
	char* end;
	char* start = strtok_r(next, "-", &end);

	if(start) {
		cpu_start = strtol(start, NULL, 16);
		cpu_end = cpu_start;
	} else
		return -1;

	if(end) {
		cpu_end = strtol(end, NULL, 16);
		if(cpu_start > cpu_end)
			return -2;
	}

	return 0;
}

static void manager_sync() {
	shared->mp_cores[0] = 0;
}


static char _boot_command_line[1024] = {0,};
static int parse_args(char* args) {
	int fd = open(args, O_RDONLY);
	if(fd < 0)
		return -1;
	ssize_t len = read(fd, _boot_command_line, 1023);
	if(!len)
		return -2;
	close(fd);

	//TODO Don't need
// 	printf("Copying Command Line...\n");
// 	save_cmd_line(_boot_command_line, strlen(_boot_command_line));

	char* next = strtok(_boot_command_line, "\n");
	char* str;
	bool smap_fixed = false;
	while((str = strtok_r(next, " ", &next))) {
		char* value;
		char* key = strtok_r(str, "=", &value);

		if(!strcmp(key, "present_mask")) {
			int ret = parse_present_mask(value);
			if(ret)
				return ret;
		} else if(!strcmp(key, "memmap")) {
			int ret = smap_update_memmap(value);
			if(ret)
				return ret;
			smap_fixed = true;
		} else if(!strcmp(key, "mem")){
			int ret = smap_update_mem(value);
			if(ret)
				return ret;
			smap_fixed = true;
		}	
	}
	if(smap_fixed) {
		printf("\nSystem Memory Map Updated\n");
		smap_dump();
	}

	return 0;
}

void* kernel_start_address; //Kenrel Start address
void* rd_start_address; 	//Ramdisk Start address

static int parse_param(char* param) {
	int fd = open(param, O_RDONLY);
	if(fd < 0)
		return -1;
	char buf[128] = {0,};
	ssize_t len = read(fd, buf, 127);
	if(!len)
		return -2;
	close(fd);

	char* next = buf;
	char* elf = strtok_r(next, " ", &next);
	char* args = strtok_r(next, " ", &next);
	char* cpu = strtok_r(next, " ", &next); // TODO No need
	char* _kernel_start_address = strtok_r(next, " ", &next);
	char* _rd_start_address = strtok_r(next, " ", &next); //TODO: No need

	int ret = parse_args(args);
	if(ret)
		return ret;

	char* e;
	kernel_start_address = (long long)strtol(_kernel_start_address, &e, 16);
	//TODO: check alignment 2Mbyte
	rd_start_address = (long long)strtol(_rd_start_address, &e, 16);
	//TODO: NO Need ramdisk

	printf("\n");
	printf("\tPenguinNgin ELF File:\t%s\n", elf);
	printf("\tArgument File:\t\t%s\n", args);
	printf("\tKernel Start address:\t%p\n", kernel_start_address);
	printf("\tRamDisk Start address:\t%p\n", rd_start_address);
	printf("\tCPU Usage:\t\t%d - %d\n", cpu_start, cpu_end);

	return 0;
}

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("usage: manager file.param\n");
		return -1;
	}

	printf("\nLoad E820 Memory Map\n");
	if(smap_init() < 0)
		goto error;

	printf("\nParse Parameter...\n");
	if(parse_param(argv[1])) {
		return -2;
	}

	printf("\nInitializing memory mapping...\n");
	if(mapping_init() < 0)
		goto error;

	//TODO Copy RamDisk
	//TODO Read and Copy(kexec -l) Elf
  	if(elf_load() < 0)
  		goto error;

	int dummy_entry() {
		int fd = open_mem(O_RDWR);
		struct boot_params* _boot_params = map_boot_param(fd);
		if(!_boot_params) {
			printf("Can't Open Boot Parameter\n");
			return -1;
		}
		unsigned long symbol_addr = elf_get_symbol("boot_params");
		if(!symbol_addr) {
			printf("Can't Get Symbol Address: \"boot_params\"\n");
			return -2;
		}
		struct boot_params* boot_params = (struct boot_params*)VIRTUAL_TO_PHYSICAL(symbol_addr);
		printf("boot_params: %p\n", boot_params);
		memcpy(boot_params, _boot_params, sizeof(struct boot_params)); 
		unmap_boot_param(_boot_params);
		close_mem(fd);

		symbol_addr = elf_get_symbol("boot_command_line");
		if(!symbol_addr) {
			printf("Can't Get Symbol Address: \"boot_command_line\"");
		}
		char* boot_command_line = (char*)VIRTUAL_TO_PHYSICAL(symbol_addr);
		printf("boot_command_line: %p\n", boot_command_line);
		strcpy(boot_command_line, _boot_command_line);

		symbol_addr = elf_get_symbol("PHYSICAL_OFFSET");
		if(!symbol_addr) {
			printf("Can't Get Symbol Address: \"PHYSICAL_OFFSET\"\n");
			return -2;
		}
		uint64_t* _PHYSICAL_OFFSET = (uint64_t*)VIRTUAL_TO_PHYSICAL(symbol_addr);
		printf("PHYSICAL_OFFSET: %p\n", _PHYSICAL_OFFSET);
		*_PHYSICAL_OFFSET = (uint64_t)kernel_start_address; //TODO fix here

		return 0;
	}
	printf("\nDummy Entry Initialization\n");
	dummy_entry();

	mp_init0();
	printf("\nInitializing shared memory area...\n");
	shared_init();

	printf("\nInitializing malloc area...\n");
 	malloc_init();

	printf("\nInitializing mulitiprocessing...\n");
	mp_init(kernel_start_address);
	mp_sync0();

	printf("\nInitializing cpu...\n");
 	cpu_init();
 
 	//TODO Fix Ramdisk Size
 	printf("\nInitializing gmalloc area...\n");
 	//gmalloc_init(rd_start_address, 0x800000);
 	gmalloc_init();

	//TODO Fix Timer init libary
	//How to get frequency per second in linux?
	//timer_init(cpu_brand);

 	printf("\nInitilizing GDT...\n");
 	gdt_init();
 	printf("\nInitilizing TSS...\n");
 	tss_init();
 	printf("\nInitilizing IDT...\n");
 	idt_init();
 	mp_sync(); //barrior #2
 
 	printf("\nInitailizing local APIC...\n");
 	if(apic_init() < 0)
 		goto error;
 
 	printf("\nInitializing events...\n");
	event_init();

 	printf("\nInitializing inter-core communications...\n");
 	icc_init();
 
 	printf("Initializing linux socket device...\n");
 	socket_init();
 
 	uint16_t nic_count = device_count(DEVICE_TYPE_NIC);
 	printf("Initializing NICs: %d\n", nic_count);
 	init_nics(nic_count);
 
 	printf("Initializing VM manager...\n");
 	vm_init();
 
 	printf("Initializing RPC manager...\n");
 	manager_init();
 
 	printf("Initializing shell...\n");
 	shell_init();
 
 	event_idle_add(idle0_event, NULL);
	/*
	 *apic_write64(APIC_REG_ICR, ((uint64_t)1 << 56) |
	 *                APIC_DSH_NONE |
	 *                APIC_TM_EDGE |
	 *                APIC_LV_DEASSERT |
	 *                APIC_DM_PHYSICAL |
	 *                APIC_DMODE_FIXED |
	 *                48);
	 */
	while(1)
		event_loop();

	return 0;
error:
	printf("Initial memory mapping error occured. Terminated...\n");
	return -1;
}
