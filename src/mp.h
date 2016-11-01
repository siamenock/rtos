#ifndef __MP_H__
#define __MP_H__

#include <stdint.h>
#include <stdbool.h>

#define LINUX_BSP_APIC_ID           0
#define PACEKTNGIN_BSP_APIC_ID      1
#define BSP_APIC_ID_OFFSET          1

#define MP_MAX_CORE_COUNT	16
#define MP_CORE_INVALID		255

#define MP_CORE(ptr, id)        (void*)((uint64_t)(ptr) + id * 0x200000)

void mp_init0();
void mp_init(unsigned long kernel_start_address);
uint8_t mp_apic_id();
uint8_t mp_core_id();
uint8_t mp_apic_id_to_core_id(uint8_t apic_id);
uint8_t mp_core_count();

void mp_sync0();
void mp_sync();
uint8_t* mp_core_map();

#endif /* __MP_H__ */
