#ifndef __ASM_BARRIER_H__
#define __ASM_BARRIER_H__

// CONFIG_X86_PPRO_FENCE undefined & CONFIG_SMP defined & CONFIG_X86_32 undefined
#define wmb()		asm volatile("sfence" ::: "memory")
#define rmb()		asm volatile("lfence" ::: "memory")
#define mb()		asm volatile("mfence" ::: "memory")

#define barrier() __asm__ __volatile__("": : :"memory")

#define dma_rmb()	barrier()
#define dma_wmb()	barrier()

#define smp_wmb()	barrier()
#define smp_rmb()	dma_rmb()	
#define smp_mb()	mb()

#endif /* __ASM_GENERIC_BARRIER_H__ */
