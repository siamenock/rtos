#ifndef ___TIMER_H__
#define ___TIMER_H__

#include <stdint.h>

void time_init();
uint64_t timer_frequency();

uint64_t time_s();
uint64_t time_ms();
uint64_t time_us();
uint64_t time_ns();

void time_swait(uint32_t s);	
void time_mwait(uint32_t ms);
void time_uwait(uint32_t us);
void time_nwait(uint32_t ns);

extern const uint64_t TIMER_FREQUENCY_PER_SEC;

#endif /* __TIMER_H__ */
