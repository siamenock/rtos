#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "mmap.h"
#include "shared.h"

#define PHYSICAL_OFFSET		0x38000000l	/* 896 MB */

#define MAPPING_AREA		(PHYSICAL_OFFSET + DESC_TABLE_AREA_START)
// TODO: gmalloc can be expanded til over 2G when bmalloc area added
#define MAPPING_AREA_SIZE	0x80000000	/* 2 GB */

void* mapping;
void* gmalloc_pool;

int mapping_init() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}

	printf("Assuming physical mapping area : %lx\n", MAPPING_AREA);

	/* Mapping area : 1 MB */
	mapping = mmap((void*)DESC_TABLE_AREA_START, MAPPING_AREA_SIZE, PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, (off_t)MAPPING_AREA);

	if(mapping == MAP_FAILED) {
		perror("Mapping memory for absolute memory access failed.\n");
		return -1;
	} else if(mapping != (void*)DESC_TABLE_AREA_START) {
		printf("Mapping memory (%p) is not same as dedicated memory (%p).\n",
				mapping, (void*)DESC_TABLE_AREA_START);

		munmap(mapping, MAPPING_AREA_SIZE);
		return -1;
	}

	printf("Memory mapped area : %lx ~ %lx\n", (uint64_t)mapping,
			(uint64_t)mapping + MAPPING_AREA_SIZE);

	return 0;
}
