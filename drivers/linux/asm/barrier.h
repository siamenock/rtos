#ifndef __ASM_BARRIER_H__
#define __ASM_BARRIER_H__

#define mb()    asm volatile("mfence":::"memory")
#define rmb()   asm volatile("lfence":::"memory")
#define wmb()   asm volatile("sfence" ::: "memory")

#endif /* __ASM_GENERIC_BARRIER_H__ */
