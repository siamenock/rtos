#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "smap.h"

int cpu_start;
int cpu_end;

static int parse_present_mask(char* present_mask) {
	char* next = present_mask;
	char* end = NULL;
	char* start = strtok_r(next, "-", &end);

	if(start) {
		cpu_start = strtol(start, NULL, 10);
		cpu_end = cpu_start;
	} else
		return -1;

	if(*end) {
		cpu_end = strtol(end, NULL, 10);
		if(cpu_start > cpu_end)
			return -2;
	}

	return 0;
}

static char _boot_command_line[1024] = {0,};
static int parse_args(char* args) {
	printf("%s\n", args);
	int fd = open(args, O_RDONLY);
	if(fd < 0) {
		perror("Failed to open ");
		return -1;
	}

	ssize_t len = read(fd, _boot_command_line, 1023);
	if(!len)
		goto error;

	close(fd);

	char* next = strtok(_boot_command_line, "\n");
	char* str;
	bool smap_fixed = false;
	while(next && (str = strtok_r(next, " ", &next))) {
		char* value = NULL;
		char* key = strtok_r(str, "=", &value);

		if(!strcmp(key, "present_mask")) {
			int ret = parse_present_mask(value);
			if(ret)
				goto error;
		} else if(!strcmp(key, "memmap")) {
			int ret = smap_update_memmap(value);
			if(ret)
				goto error;
			smap_fixed = true;
		} else if(!strcmp(key, "mem")){
			int ret = smap_update_mem(value);
			if(ret)
				goto error;
			smap_fixed = true;
		}
	}

	if(smap_fixed) {
		printf("\tSystem Memory Map Updated\n");
		smap_dump();
	}

	printf("\tCPU Usage: %d - %d\n", cpu_start, cpu_end);

	return 0;

error:
	printf("\tFailed to parse arguments\n");
	close(fd);
	return -2;
}

long kernel_start_address;	// Kernel Start address
long rd_start_address; 	// Ramdisk Start address
char kernel_elf[256];
char kernel_args[256];

int param_parse(char* params) {
	int ret;
	int fd = open(params, O_RDONLY);
	if(fd < 0)
		return -1;
	char buf[128] = {0,};
	ssize_t len = read(fd, buf, 127);
	if(!len)
		return -2;
	close(fd);

	char* next = buf;
	char* param = strtok_r(next, " ", &next);
	strcpy(kernel_elf, param);

	param = strtok_r(next, " ", &next);
	strcpy(kernel_args, param);

	// TODO: No need start core
	param = strtok_r(next, " ", &next);

	// TODO: check alignment 2Mbyte
	param = strtok_r(next, " ", &next);
	kernel_start_address = strtol(param, NULL, 16);

	// TODO: NO Need ramdisk
	param = strtok_r(next, " ", &next);
	rd_start_address = strtol(param, NULL, 16);

	printf("\tPenguinNgin ELF File:\t%s\n", kernel_elf);
	printf("\tArgument File:\t\t%s\n", kernel_args);
	//printf("\tStart Core:\t\t%d\n", start_core);
	printf("\tKernel Start address:\t%p\n", kernel_start_address);
	printf("\tRamDisk Start address:\t%p\n", rd_start_address);

	ret = parse_args(kernel_args);
	if(ret) {
		printf("\tFailed to parse kernel arguments : %d\n", ret);
		return ret;
	}

	return 0;
}

