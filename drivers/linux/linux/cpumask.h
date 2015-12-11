#ifndef __LINUX_CPU_MASK_H__
#define __LINUX_CPU_MASK_H__

#define for_each_cpu(cpu, mask)			\
	for ((cpu) = 0; (cpu) < 1; (cpu)++, (void)mask)
#define for_each_cpu_not(cpu, mask)		\
	for ((cpu) = 0; (cpu) < 1; (cpu)++, (void)mask)
#define for_each_cpu_and(cpu, mask, and)	\
	for ((cpu) = 0; (cpu) < 1; (cpu)++, (void)mask, (void)and)

#define for_each_possible_cpu(cpu) for_each_cpu((cpu), cpu_possible_mask)

#if NR_CPUS > 1
#define num_online_cpus()	cpumask_weight(cpu_online_mask)
#else
#define num_online_cpus()	1U
#endif /* NR_CPUS */

#endif /* __LINUX_CPU_MASK_H__ */
