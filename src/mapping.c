#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "smap.h"
#include "mmap.h"
#include "shared.h"

unsigned long mapping_area_size;
#define MAPPING_AREA_SIZE	mapping_area_size	/* mem - kernel_start_address */

void* gmalloc_pool;

int mapping_init() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}

	for(uint8_t i = 0 ; i < smap_count; i++) {
		if(smap[i].type != SMAP_TYPE_MEMORY)
			continue;

		void* mapping = mmap((void*)smap[i].base, smap[i].length, PROT_READ|PROT_WRITE|PROT_EXEC,
				MAP_SHARED, fd, smap[i].base);

		if(mapping == MAP_FAILED) {
			perror("Mapping memory for absolute memory access failed.\n");
			return -1;
		} else if(mapping != (void*)smap[i].base) {
			printf("Mapping memory (%p) is not same as dedicated memory.\n",
					mapping);

			munmap(mapping, MAPPING_AREA_SIZE);
			return -1;
		}
		mapping_area_size += smap[i].length;
	}

	printf("Assuming physical mapping area : %lx\n", mapping_area_size);

	return 0;
}
