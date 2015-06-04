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

#define CPU_INFOS_SIZE	0x17
#define CPU_EXTENDED_INFOS_SIZE	0x9
extern uint32_t cpu_infos[CPU_INFOS_SIZE][4];
extern uint32_t cpu_extended_infos[CPU_EXTENDED_INFOS_SIZE][4];

void cpu_init();
void cpu_info();
uint64_t cpu_tsc();
void cpu_swait(uint32_t s);
void cpu_mwait(uint32_t ms);
void cpu_uwait(uint32_t us);
void cpu_nwait(uint32_t ns);

#define CPU_IS_SSE_4_1		!!(cpu_infos[0x1][2] & 0x80000)
#define CPU_IS_SSE_4_2		!!(cpu_infos[0x1][2] & 0x100000)
#define CPU_IS_MONITOR_MWAIT	!!(cpu_infos[0x1][2] & 0x8)
#define CPU_IS_MWAIT_INTERRUPT	!!(cpu_infos[0x5][2] & 0x2)
#define CPU_IS_TURBO_BOOST	!!(cpu_infos[0x6][0] & 0x2)
#define CPU_IS_INVARIANT_TSC	!!(cpu_extended_infos[0x7][3] & 0x100)

#endif /* __CPU_H__ */
