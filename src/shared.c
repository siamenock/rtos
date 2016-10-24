#include <stdio.h>
#include "shared.h"
#include "mmap.h"

Shared* shared;

int shared_init() {
	/* Shared area : 2 MB - sizeof(Shared) */
	shared = (Shared*)(KERNEL_TEXT_AREA_START- sizeof(Shared));

	if(shared->magic != SHARED_MAGIC) {
		printf("Shared area magic number is not same as PacketNgin kernel\n");
		return -1;
	}

	return 0;
}
