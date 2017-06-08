#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "smap.h"
#include "msr.h"
#include "mmap.h"
#include "shared.h"
#include "page.h"

static inline int open_memory() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}
	return fd;
}

int mapping_memory() {
#define MAPPING_START	(void*)0x100000
#define MAPPING_AREA	PHYSICAL_OFFSET + 0x100000
//#define MAPPING_START	(void*)VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START)
//#define MAPPING_AREA		(PHYSICAL_OFFSET + VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START))
// TODO: gmalloc can be expanded til over 2G when bmalloc area added
#define MAPPING_AREA_SIZE	0x24000000	/* 2 GB */
	int fd = open_memory();
	if(fd < 0)
		return -1;

	printf("\tPhysical offset : %p\n", PHYSICAL_OFFSET);
	printf("\tAssuming physical mapping area : %lx\n", MAPPING_AREA);

	/* Mapping area : 1 MB */
	void* mapping = mmap(MAPPING_START,
			MAPPING_AREA_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_SHARED, fd, (off_t)MAPPING_AREA);

	if(mapping == MAP_FAILED) {
		perror("\tMapping memory for absolute memory access failed.\n");
		return -1;
	} else if(mapping != MAPPING_START) {
		printf("\tMapping memory (%p) is not same as dedicated memory (%p).\n",
				mapping, (void*)VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START));

		munmap(mapping, MAPPING_AREA_SIZE);
		return -1;
	}

	printf("\tMemory mapped area : %lx ~ %lx\n", (uint64_t)mapping,
			(uint64_t)mapping + MAPPING_AREA_SIZE);
	close(fd);

	return 0;
}

int mapping_apic() {
	int fd = open_memory();
	if(fd < 0)
		return -1;

	extern uint64_t _apic_address;
	_apic_address = msr_read(MSR_IA32_APIC_BASE) & 0xFFFFF000;
	printf("\tAssuming APIC physical base: %lx \n", _apic_address & ~(uint64_t)0xfffff);

	uint64_t _apic_address_page = (uint64_t)mmap((void*)(_apic_address & ~(uint64_t)0xfffff), PAGE_PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)(_apic_address & ~(uint64_t)0xfffff));

	if(_apic_address_page == (uint64_t)MAP_FAILED) {
		perror("\tMapping memory for absolute memory access failed.\n");
		return -1;
	}

	if(_apic_address_page != _apic_address) {
		perror("\tMapping memory is not same as APIC address.\n");
		return -1;
	}

	printf("\tMemory mapped APIC physcial base: %lx \n", _apic_address);
	close(fd);
	return 0;
}

