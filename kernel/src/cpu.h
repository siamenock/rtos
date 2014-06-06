#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

extern uint64_t cpu_frequency;

void cpu_init();
void cpu_info();
uint64_t cpu_tsc();
void cpu_swait(uint32_t s);
void cpu_mwait(uint32_t ms);
void cpu_uwait(uint32_t us);
void cpu_nwait(uint32_t ns);

#endif /* __CPU_H__ */
