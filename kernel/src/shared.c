#include <string.h>
#include "shared.h"

Shared* shared = (Shared*)(0xffffffff80200000 - sizeof(Shared));

/*
void shared_init() {
	bzero(shared, sizeof(Shared));
}
*/
