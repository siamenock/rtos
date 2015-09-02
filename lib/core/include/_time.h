#ifndef ___TIME_H__
#define ___TIME_H__

#include <time.h>
#include <stdint.h>

#ifndef time_t
typedef long int time_t;
#endif
#ifndef time_t
typedef long int clock_t;
#endif

void time_init();
uint64_t cpu_tsc();
time_t time_s();
time_t time_ms();
time_t time_us();
time_t time_ns();

void cpu_swait(uint32_t s);
void cpu_mwait(uint32_t ms);
void cpu_uwait(uint32_t us);
void cpu_nwait(uint32_t ns);

extern uint64_t cpu_ms;
extern uint64_t cpu_us;
extern uint64_t cpu_ns;
extern clock_t cpu_clock;
extern uint64_t cpu_frequency;

#endif /* ___TIME_H__ */
