#ifndef __TIME_H__
#define __TIME_H__

#include <stdint.h>

void time_init();
void time_swait(uint32_t s);
void time_mwait(uint32_t ms);
void time_uwait(uint32_t us);
void time_nwait(uint32_t ns);

#endif /* __TIME_H__ */

