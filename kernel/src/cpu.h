#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>
#include <time.h>

extern uint64_t cpu_frequency;
extern uint64_t cpu_ms;
extern uint64_t cpu_us;
extern uint64_t cpu_ns;
extern clock_t cpu_clock;
extern char cpu_brand[4 * 4 * 3 + 1];

void cpu_init();
void cpu_info();
uint64_t cpu_tsc();
void cpu_swait(uint32_t s);
void cpu_mwait(uint32_t ms);
void cpu_uwait(uint32_t us);
void cpu_nwait(uint32_t ns);

#endif /* __CPU_H__ */
