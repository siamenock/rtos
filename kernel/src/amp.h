#ifndef __AMP_H___
#define __AMP_H___
#include <stdint.h> 

void cpuid(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d);
uint8_t get_apic_id();

#endif /*__AMP_H__*/
