#include <stdio.h>
#include "mp.h"
#include "port.h"
#include "apic.h"
#include "ioapic.h"

uint64_t _ioapic_address;

static uint8_t redirection_map[24];

void ioapic_init() {
	bool init_IOInterruptEntry(MP_IOInterruptEntry* entry) {
		if(entry->interrupt_type == 0x00 && entry->source_bus_id == mp_bus_isa_id()) {	// interrut_type == Interrupt
			uint64_t redirection = 	((uint64_t)32 + entry->source_bus_irq) |
						APIC_DM_PHYSICAL |
						APIC_DMODE_FIXED |
						APIC_PP_ACTIVEHIGH |
						APIC_TM_EDGE |
						APIC_IM_ENABLED |
						((uint64_t)(entry->source_bus_irq == 0 ? 0xff : 0x00) << 56);
			
			ioapic_write64(IOAPIC_IDX_REDIRECTION_TABLE + entry->destination_io_apic_intin * 2, redirection);
			redirection_map[entry->source_bus_irq] = entry->destination_io_apic_intin;
			//printf("ISA IRQ remap: %d -> %d to %s\n", entry->destination_io_apic_intin, 32 + entry->source_bus_irq, entry->source_bus_irq == 0 ? "all" : "core 0");
		}
		return true;
	}
	
	// Disable PIC mode
	if(mp_FloatingPointerStructure()->feature[1] & 0x80) {
		printf("\tDisable PIC mode...\n");
		port_out8(0x22, 0x70);
		port_out8(0x23, 0x01);
	}
	
	// Mask all interrupt first
	for(int i = 0; i < 24; i++) {
		redirection_map[i] = 0xff;
		
		uint64_t redirection = ioapic_read64(IOAPIC_IDX_REDIRECTION_TABLE + i * 2);
		redirection |= APIC_IM_DISABLED;
		ioapic_write64(IOAPIC_IDX_REDIRECTION_TABLE + i * 2, redirection);
	}
	
	// Redirect interrupt next
	mp_iterate_IOInterruptEntry(init_IOInterruptEntry);
}

inline uint32_t ioapic_read32(int idx) {
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx;
	return *(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW);
}

inline void ioapic_write32(int idx, uint32_t v) {
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx;
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW) = v;
}

inline uint64_t ioapic_read64(int idx) {
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx;
	uint64_t v = (uint64_t)*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW);
	
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx + 1;
	v |= (uint64_t)*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW) << 32;
	
	return v;
}

inline void ioapic_write64(int idx, uint64_t v) {
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx;
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW) = (uint32_t)v;
	
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_SELECT) = idx + 1;
	*(uint32_t volatile*)(_ioapic_address + IOAPIC_REG_WINDOW) = (uint32_t)(v >> 32);
}
