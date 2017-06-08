#ifndef __MSR_H__
#define __MSR_H__

#include <stdint.h>

#define MSR_IA32_APIC_BASE	0x1B
#define MSR_IA32_PERF_STATUS	0x198
#define MSR_IA32_PERF_CTL	0x199

/**
 * @file MSR(Model Specific Register Intrinsics)
 * @see Intel64 Architectures Devlopers's Manual Chapter35
 */

/**
  * Read a value from MSR
  * @param register_address Address of a target MSR
  * @return The contents of MSR (EDX:EAX)
  */
uint64_t msr_read(uint32_t register_address);

/**
 * Write a value into MSR
 * @param value value
 * @param register_address Address of a target MSR
 */
void msr_write(uint64_t value, uint32_t register_address);

/**
 * Write a value into MSR
 * @param dx dx register value
 * @param ax ax register value
 * @param register_address Address of a target MSR
 */
void msr_write2(uint32_t dx, uint32_t ax, uint32_t register_address);

#endif
