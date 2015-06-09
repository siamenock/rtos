#ifndef __MP_H__
#define __MP_H__

#include <stdint.h>
#include <stdbool.h>

#define MP_MAX_CORE_COUNT	16

#define MP_CORE(ptr, id)	(void*)((uint64_t)(ptr) + id * 0x200000)

typedef struct {
	uint8_t		signature[4];
	uint32_t	physical_address_pointer;
	uint8_t		length;
	uint8_t		spec_rev;
	uint8_t		checksum;
	uint8_t		feature[5];
} __attribute__((packed)) MP_FloatingPointerStructure;

typedef struct {
	uint8_t		signature[4];
	uint16_t	base_table_length;
	uint8_t		spec_rev;
	uint8_t		checksum;
	uint8_t		oem_id[8];
	uint8_t		product_id[12];
	uint32_t	oem_table_pointer;
	uint16_t	oem_table_size;
	uint16_t	entry_count;
	uint32_t	local_apic_address;
	uint16_t	extended_table_length;
	uint8_t		extended_table_checksum;
	uint8_t		reserved;
} __attribute__((packed)) MP_ConfigurationTableHeader;

typedef struct {
	uint8_t		type;	// 0
	uint8_t		local_apic_id;
	uint8_t		local_apic_version;
	uint8_t		cpu_flags;
	uint32_t	cpu_signature;
	uint32_t	feature_flags;
	uint32_t	reserved[2];
} __attribute__((packed)) MP_ProcessorEntry;

typedef struct {
	uint8_t		type;	// 1
	uint8_t		bus_id;
	uint8_t		bus_type[6];
} __attribute__((packed)) MP_BusEntry;

typedef struct {
	uint8_t		type;	// 2
	uint8_t		io_apic_id;
	uint8_t		io_apic_version;
	uint8_t		io_apic_flags;
	uint32_t	io_apic_address;
} __attribute__((packed)) MP_IOAPICEntry;

typedef struct {
	uint8_t		type;	// 3
	uint8_t		interrupt_type;
	uint16_t	io_interrupt_flag;
	uint8_t		source_bus_id;
	uint8_t		source_bus_irq;
	uint8_t		destination_io_apic_id;
	uint8_t		destination_io_apic_intin;
} __attribute__((packed)) MP_IOInterruptEntry;

typedef struct {
	uint8_t		type;	// 4
	uint8_t		interrupt_type;
	uint16_t	local_interrupt_flag;
	uint8_t		source_bus_id;
	uint8_t		source_bus_irq;
	uint8_t		destination_local_apic_id;
	uint8_t		destination_local_apic_intin;
} __attribute__((packed)) MP_LocalInterruptEntry;

typedef struct {
	uint8_t		type;	// 128
	uint8_t		entry_length;
	uint8_t		bus_id;
	uint8_t		address_type;
	uint64_t	address_base;
	uint64_t	address_length;
} __attribute__((packed)) MP_SystemAddressSpaceEntry;

typedef struct {
	uint8_t		type;	// 129
	uint8_t		entry_length;
	uint8_t		bus_id;
	uint8_t		bus_info;
	uint8_t		parent_bus;
	uint8_t		reserved[3];
} __attribute__((packed)) MP_BusHierarchyDescriptorEntry;

typedef struct {
	uint8_t		type;	// 130
	uint8_t		entry_length;
	uint8_t		bus_id;
	uint8_t		address_mod;
	uint32_t	predefined_range_list;
} __attribute__((packed)) MP_CompatabilityBusAddressSpaceModifierEntry;

extern bool mp_apics[];

void mp_init0();
void mp_analyze();
void mp_init();
void mp_wait_init();
bool mp_wait(uint8_t core_id, uint32_t us);
void mp_wakeup();
void mp_sync();
uint8_t mp_core_count();
uint8_t mp_thread_count();
uint8_t mp_core_id();
uint8_t mp_apic_id();
uint8_t mp_core_id_to_apic_id(uint8_t core_id);
uint8_t mp_bus_isa_id();
uint8_t mp_bus_pci_id();
MP_FloatingPointerStructure* mp_FloatingPointerStructure();
void mp_iterate_IOInterruptEntry(bool(*fn)(MP_IOInterruptEntry*));

#endif /* __MP_H__ */
