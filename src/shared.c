#include <stdio.h>
#include "shared.h"
#include "mmap.h"

Shared* shared;

void shared_init() {
	/* Shared area : 2 MB - sizeof(Shared) */
	shared = (Shared*)(DESC_TABLE_AREA_END - sizeof(Shared));
}
