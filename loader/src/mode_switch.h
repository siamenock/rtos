#ifndef __MODE_SWITCH_H__
#define __MODE_SWITCH_H__

#include <stdint.h>

void read_CPUID(uint32_t v, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);
void mode_switch();

#endif /* __MODE_SWITCH_H__ */
