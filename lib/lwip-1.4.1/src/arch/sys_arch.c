#include <time.h>
#include "lwip/def.h"

u32_t sys_now(void) {
	return (u32_t)(clock() / (CLOCKS_PER_SEC / 1000));
}
