#include <stdio.h>
#include <string.h>
#include "mp.h"
#include "port.h"
#include "apic.h"
#include "acpi.h"
#include "ioapic.h"

uint64_t _ioapic_address;
static uint8_t redirection_map[24];

static bool parse_fps(MP_FloatingPointerStructure* entry, void* context) {
	*(uint8_t*)context = entry->feature[1];

	return false;
}

static bool parse_be(MP_BusEntry* entry, void* context) {
	if(memcmp(entry->bus_type, "ISA", 3) == 0) {
		*(uint8_t*)context = entry->bus_id;
		
		return false;
	}
	
	return true;
}

static bool parse_iie(MP_IOInterruptEntry* entry, void* context) {
	uint8_t bus_isa_id = *(uint8_t*)context;
	
	if(entry->interrupt_type == 0x00 && entry->source_bus_id == bus_isa_id) {	// interrut_type == Interrupt
		uint64_t redirection = 	((uint64_t)32 + entry->source_bus_irq) |
					APIC_DM_PHYSICAL |
					APIC_DMODE_FIXED |
					APIC_PP_ACTIVEHIGH |
					APIC_TM_EDGE |
					APIC_IM_ENABLED |
					((uint64_t)(entry->source_bus_irq == 0 ? 0xff : 0x00) << 56);
		
		ioapic_write64(IOAPIC_IDX_REDIRECTION_TABLE + entry->destination_io_apic_intin * 2, redirection);
		redirection_map[entry->destination_io_apic_intin] = entry->source_bus_irq;
		printf("\tISA IRQ remap[%d]: %d -> %d to %s\n", entry->destination_io_apic_intin, entry->source_bus_irq, 32 + entry->source_bus_irq, entry->source_bus_irq == 0 ? "all" : "core 0");
	}
	return true;
}
	
static bool parse_iso(InterruptSourceOverride* entry, void* context) {
	uint8_t destination_io_apic_intin = (uint8_t)(uint64_t)context;

	if(entry->global_system_interrupt == destination_io_apic_intin) {
		uint64_t redirection = 	((uint64_t)32 + entry->irq_source) |
					APIC_DM_PHYSICAL |
					APIC_DMODE_FIXED |
					APIC_PP_ACTIVEHIGH |
					APIC_TM_EDGE |
					APIC_IM_ENABLED |
					((uint64_t)(entry->irq_source == 0 ? 0xff : 0x00) << 56);

		ioapic_write64(IOAPIC_IDX_REDIRECTION_TABLE + entry->global_system_interrupt * 2, redirection);
		redirection_map[entry->global_system_interrupt] = entry->irq_source;
		printf("\tISA IRQ remap[%d]: %d -> %d to %s\n", entry->global_system_interrupt, entry->irq_source, 32 + entry->irq_source, entry->irq_source == 0 ? "all" : "core 0");

		return false;
	}

	return true;
}

void ioapic_init() {
	// Disable PIC mode
	uint8_t feature1;
	
	MP_Parser parser = { .parse_fps = parse_fps };
	mp_parse_fps(&parser, &feature1);
	
	if(feature1 & 0x80) {
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
	
//	// Parse ISA/PCI bus ID
	uint8_t bus_isa_id = (uint8_t)-1;
	bzero(&parser, sizeof(MP_Parser));
	parser.parse_be = parse_be;
	mp_parse_fps(&parser, &bus_isa_id);
	
	bzero(&parser, sizeof(MP_Parser));
	parser.parse_iie = parse_iie;
	mp_parse_fps(&parser, &bus_isa_id);

	ACPI_Parser acpi_parser = { .parse_apic_iso = parse_iso };
	for(int i = 0; i < 24; i++) {
		if(redirection_map[i] == 0xff) {
			acpi_parse_rsdt(&acpi_parser, (void*)(uint64_t)i);
		}
	}

	//TODO Unknown bug in Celeron
	if(redirection_map[1] == 0xff) {
		uint64_t redirection = 	((uint64_t)32 + 1) |
					APIC_DM_PHYSICAL |
					APIC_DMODE_FIXED |
					APIC_PP_ACTIVEHIGH |
					APIC_TM_EDGE |
					APIC_IM_ENABLED |
					((uint64_t)(1 == 0 ? 0xff : 0x00) << 56);

		ioapic_write64(IOAPIC_IDX_REDIRECTION_TABLE + 1 * 2, redirection);
		redirection_map[1] = 1;
		printf("\tISA IRQ remap[%d]: %d -> %d to %s\n", 1, 1, 32 + 1, 1 == 0 ? "all" : "core 0");
	}
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
