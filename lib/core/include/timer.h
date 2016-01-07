#ifndef ___TIMER_H__
#define ___TIMER_H__

#include <stdint.h>

extern const uint64_t TIMER_FREQUENCY_PER_SEC;

void timer_init(const char* cpu_brand);
uint64_t timer_frequency();

uint64_t timer_s();
uint64_t timer_ms();
uint64_t timer_us();
uint64_t timer_ns();

void timer_swait(uint32_t s);	
void timer_mwait(uint32_t ms);
void timer_uwait(uint32_t us);
void timer_nwait(uint32_t ns);

#endif /* __TIMER_H__ */
