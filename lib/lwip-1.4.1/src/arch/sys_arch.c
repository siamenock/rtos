#include <timer.h>
#include "lwip/def.h"

u32_t sys_now(void) {
	return (u32_t)timer_ms();
}
