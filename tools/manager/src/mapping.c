#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "smap.h"
#include "mmap.h"
#include "shared.h"
#include "page.h"

unsigned long mapping_area_size;
/*
 *unsigned long mapping_area_size;
 *#define MAPPING_AREA_SIZE	mapping_area_size	[> mem - kernel_start_address <]
 */

void* gmalloc_pool;

uint8_t		smap_reserved_count;
SMAP		smap_reserved[SMAP_MAX];

uint8_t		virtual_smap_count;
SMAP		virtual_smap[SMAP_MAX];

#define LINUX_PAGE_SIZE		4096
#define ALIGN(addr)		((addr) & ~((uint64_t)LINUX_PAGE_SIZE - 1))

int mapping_physical(SMAP* smap) {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}

	for(uint8_t i = 0 ; i < smap_count; i++) {
		if(smap[i].type != SMAP_TYPE_MEMORY)
			continue;

		void* mapping = mmap((void*)ALIGN(smap[i].base), 
				ALIGN(smap[i].base + smap[i].length + 4096 - 1) 
				- ALIGN(smap[i].base), PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_SHARED, fd, (__off_t)ALIGN(smap[i].base));

		if(mapping == MAP_FAILED) {
			perror("Mapping memory for absolute memory access failed.\n");
			close(fd);
			return -1;
		} else if(mapping != (void*)ALIGN(smap[i].base)) {
			printf("Mapping memory (%p) is not same as dedicated memory.\n",
					mapping);

			munmap(mapping, ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base));
			close(fd);
			return -1;
		}

		mapping_area_size += ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base);
	}
	close(fd);

	return 0;
}

void mapping_destroy(SMAP* smap) {
	for(uint8_t i = 0 ; i < smap_count; i++) {
		if(smap[i].type != SMAP_TYPE_MEMORY)
			continue;

		void* mapping = (void*)ALIGN(smap[i].base);
		munmap(mapping, ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base));
		mapping_area_size -= ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base);
	}
}

int mapping_reserved() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}

	for(uint8_t i = 0 ; i < smap_count; i++) {
		if(smap[i].type != SMAP_TYPE_RESERVED)
			continue;

		void* mapping = mmap((void*)ALIGN(smap[i].base), ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base), PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_SHARED, fd, (__off_t)ALIGN(smap[i].base));

		if(mapping == MAP_FAILED) {
			perror("Mapping memory for absolute memory access failed.\n");
			return -1;
		} else if(mapping != (void*)ALIGN(smap[i].base)) {
			printf("Mapping memory (%p) is not same as dedicated memory.\n",
					mapping);

			munmap(mapping, ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base));
			return -1;
		}
		smap_reserved[smap_reserved_count].base = ALIGN(smap[i].base);
		smap_reserved[smap_reserved_count].length = ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base);
		smap_reserved[smap_reserved_count].type = SMAP_TYPE_RESERVED;
		smap_reserved_count++;

		mapping_area_size += ALIGN(smap[i].base + smap[i].length + 4096 - 1) - ALIGN(smap[i].base);
	}

	return 0;
}

bool is_specific_space(void* base_addr) {
	bool check(uint64_t addr1_start, uint64_t addr1_end, uint64_t addr2) {
		uint64_t start_idx = addr1_start & ~(PAGE_PAGE_SIZE - 1);
		uint64_t end_idx = (addr1_end  + (PAGE_PAGE_SIZE - 1)) & ~(PAGE_PAGE_SIZE - 1);
		uint64_t base_idx = addr2 & ~(PAGE_PAGE_SIZE - 1);

		if(start_idx <= base_idx && base_idx < end_idx) {
			return true;
		}

		return false;
	}

	for(int i = 0; i < smap_reserved_count; i++) {
		if(check(smap_reserved[i].base, smap_reserved[i].base + smap_reserved[i].length, 
					(uint64_t)base_addr))
			return true;
	}

	return false;
}

static void virtual_smap_dump() {
	for(uint8_t i = 0 ; i < virtual_smap_count; i++) {
		SMAP* entry = &virtual_smap[i];
		char* type;
		switch(entry->type) {
			case SMAP_TYPE_MEMORY:
				type = "Memory";
				break;
			case SMAP_TYPE_RESERVED:
				type = "Reserved";
				break;
			case SMAP_TYPE_ACPI:
				type = "ACPI";
				break;
			case SMAP_TYPE_NVS:
				type = "NVS";
				break;
			case SMAP_TYPE_UNUSABLE:
				type = "Disabled";
				break;
			default:
				type = "Unknown";
		}
		printf("\t0x%016lx - 0x%016lx: %s(%d)\n", entry->base, entry->base + entry->length, type, entry->type);
	}
}

#define MAPPING_AREA		(PHYSICAL_OFFSET + VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START))
// TODO: gmalloc can be expanded til over 2G when bmalloc area added
#define MAPPING_AREA_SIZE	0x24000000	/* 2 GB */

int mapping_memory() {
	void* mapping;
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}
	mapping_area_size = 0;

	printf("\tPhysical offset : %p\n", PHYSICAL_OFFSET);
	printf("\tAssuming physical mapping area : %lx\n", MAPPING_AREA);

	/* Mapping area : 1 MB */
	mapping = mmap((void*)VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START),
			MAPPING_AREA_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
			MAP_SHARED, fd, (off_t)MAPPING_AREA);

	if(mapping == MAP_FAILED) {
		perror("\tMapping memory for absolute memory access failed.\n");
		return -1;
	} else if(mapping != (void*)VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START)) {
		printf("\tMapping memory (%p) is not same as dedicated memory (%p).\n",
				mapping, (void*)VIRTUAL_TO_PHYSICAL(DESC_TABLE_AREA_START));

		munmap(mapping, MAPPING_AREA_SIZE);
		return -1;
	}

	printf("\tMemory mapped area : %lx ~ %lx\n", (uint64_t)mapping,
			(uint64_t)mapping + MAPPING_AREA_SIZE);

/*
 *        void* base_addr = 0x0;
 *        for(uint8_t i = 0 ; i < smap_count; i++) {
 *                if(smap[i].type != SMAP_TYPE_MEMORY)
 *                        continue;
 *
 *                void* base = (void*)smap[i].base;
 *                void* end = (void*)smap[i].base + smap[i].length;
 *                //TODO: check Alignment
 *                while(base < end) {
 *                        bool reserved = is_specific_space(base_addr);
 *
 *                        if(!reserved) {
 *                                void* mapping = mmap(base_addr, PAGE_PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC,
 *                                                MAP_SHARED, fd, base);
 *
 *                                if(mapping == MAP_FAILED) {
 *                                        perror("Mapping memory for absolute memory access failed.\n");
 *                                        return -1;
 *                                } else if(mapping != (void*)base_addr) {
 *                                        printf("Mapping memory (%p) is not same as dedicated memory (%p).\n", mapping, base_addr);
 *
 *                                        munmap(mapping, MAPPING_AREA_SIZE);
 *                                        return -1;
 *                                }
 *                                base += PAGE_PAGE_SIZE;
 *                                mapping_area_size += PAGE_PAGE_SIZE;
 *                        }
 *
 *                        if(virtual_smap_count == 0) {
 *                                virtual_smap[virtual_smap_count].base = 0;
 *                                virtual_smap[virtual_smap_count].length = PAGE_PAGE_SIZE;
 *                                virtual_smap[virtual_smap_count].type = reserved ? SMAP_TYPE_RESERVED: SMAP_TYPE_MEMORY;
 *                                virtual_smap_count++;
 *                        } else {
 *                                if(virtual_smap[virtual_smap_count - 1].type == (reserved ? SMAP_TYPE_RESERVED: SMAP_TYPE_MEMORY)) {
 *                                        virtual_smap[virtual_smap_count - 1].length += PAGE_PAGE_SIZE;
 *                                } else {
 *                                        virtual_smap[virtual_smap_count].base = base_addr;
 *                                        virtual_smap[virtual_smap_count].length = PAGE_PAGE_SIZE;
 *                                        virtual_smap[virtual_smap_count].type = reserved ? SMAP_TYPE_RESERVED: SMAP_TYPE_MEMORY;
 *                                        virtual_smap_count++;
 *                                }
 *                        }
 *
 *                        base_addr += PAGE_PAGE_SIZE;
 *                }
 *        }
 *
 *        printf("Assuming physical mapping area : 0x%lx\n", mapping_area_size);
 *        printf("Virtual Memory Map\n");
 *        virtual_smap_dump();
 *
 */
	return 0;
}

