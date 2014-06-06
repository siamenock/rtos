#include <string.h>
#include "shared.h"

Shared* shared = (Shared*)(0xffffffff80200000 - 0x10000);

void shared_init() {
	bzero(shared, sizeof(Shared));
}
