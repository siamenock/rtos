#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>
#include <stdbool.h>

#define PROCESSOR_LOCAL_APIC		0
#define IO_APIC				1
#define INTERRUPT_SOURCE_OVERRIDE	2
#define NONMASKABLE_INTERRUPT_SOURCE	3

typedef struct {
	uint8_t		signature[8];
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		revision;
	uint32_t	address;
} __attribute__((packed)) RSDP;

typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;
} __attribute__((packed)) ACPIHeader;

// Root system description table
typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;
	
	uint32_t	table[0];
} __attribute__((packed)) RSDT;

// Fixed ACPI Description Table
typedef struct {
	uint8_t		address_space;
	uint8_t		bit_width;
	uint8_t		bit_offset;
	uint8_t		address_size;
	uint64_t	address;
} GenericAddressStructure;

typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;
	
	uint32_t	firmware_ctrl;
	uint32_t	dsdt;
	
	// APIC 1.0
	uint8_t		reserved;
	
	uint8_t		preferred_power_management_profile;
	uint16_t	sci_interrupt;
	uint32_t	smi_command_port;
	uint8_t		acpi_enable;
	uint8_t		acpi_disable;
	uint8_t		s4bios_req;
	uint8_t		pstate_control;
	uint32_t	pm1a_event_block;
	uint32_t	pm1b_event_block;
	uint32_t	pm1a_control_block;
	uint32_t	pm1b_control_block;
	uint32_t	pm2_control_block;
	uint32_t	pm_timer_block;
	uint32_t	gpe0_block;
	uint32_t	gpe1_block;
	uint8_t		pm1_event_length;
	uint8_t		pm1_control_length;
	uint8_t		pm2_control_length;
	uint8_t		pm_timer_length;
	uint8_t		gpe0_length;
	uint8_t		gpe1_length;
	uint8_t		gpe1_base;
	uint8_t		c_state_control;
	uint16_t	worst_c2_latency;
	uint16_t	worst_c3_latency;
	uint16_t	flush_size;
	uint16_t	flush_stride;
	uint8_t		duty_offset;
	uint8_t		duty_width;
	uint8_t		day_alarm;
	uint8_t		month_alarm;
	uint8_t		century;
	
	uint16_t	boot_architecture_flags;

	uint8_t		reserved2;
	uint32_t	flags;
	
	GenericAddressStructure	reset_reg;

	uint8_t		reset_value;
	uint8_t		reserved3[3];
	
	uint64_t	x_firmware_control;
	uint64_t	x_dsdt;
	
	GenericAddressStructure	x_pm1a_event_block;
	GenericAddressStructure	x_pm1b_event_block;
	GenericAddressStructure	x_pm1a_control_block;
	GenericAddressStructure	x_pm1b_control_block;
	GenericAddressStructure	x_pm2_control_block;
	GenericAddressStructure	x_pm_timer_block;
	GenericAddressStructure	x_gpe0_block;
	GenericAddressStructure	x_gpe1_block;
} __attribute__((packed)) FADT;

typedef struct {
	uint64_t	address;
	uint16_t	group;
	uint8_t		start;
	uint8_t		end;
	uint32_t	reserved;
} __attribute__((packed)) MMapConfigSpace;

typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;
	
	uint64_t	reserved;
	MMapConfigSpace	mmaps[0];
} __attribute__((packed)) MCFG;

// Multiple APIC Description Table
// Entry Type 0: Processor Local APIC
typedef struct {
	uint8_t		entry_type;
	uint8_t		record_length;
	uint8_t		acpi_processor_id;
	uint8_t		apic_id;
	uint32_t	flags;
} __attribute__((packed)) ProcessorLocalAPIC;

// Entry Type 1: I/O APIC
typedef struct {
	uint8_t		entry_type;
	uint8_t		record_length;
	uint8_t		io_apic_id;
	uint8_t		reserved;
	uint32_t	io_apic_address;
	uint32_t	global_system_interrupt_base;
} __attribute__((packed)) IOAPIC;

// Entry Type 2: Interrupt Source Override
typedef struct {
	uint8_t		entry_type;
	uint8_t		record_length;
	uint8_t		bus_source;
	uint8_t		irq_source;
	uint32_t	global_system_interrupt;
	uint16_t	flags;
} __attribute__((packed)) InterruptSourceOverride;

//Entry Type 3: NonMaskable INterrupt Source
typedef struct {
	uint8_t		entry_type;
	uint8_t		record_length;
	uint16_t	flags;
	uint32_t	global_system_interrupt;
} __attribute__((packed)) NonMaskableInterruptSource;

typedef struct {
	uint8_t		signature[4];
	uint32_t	length;
	uint8_t		revision;
	uint8_t		checksum;
	uint8_t		oem_id[6];
	uint8_t		oem_table_id[8];
	uint32_t	oem_revision;
	uint32_t	creator_id;
	uint32_t	creator_revision;

	uint32_t	local_controller_address;
	uint32_t	flags;
	uint8_t		entry[0];
} __attribute__((packed)) MADT;

typedef struct {
	bool(*parse_rsdp)(RSDP*, void*);
	bool(*parse_rsdt)(RSDT*, void*);
	bool(*parse_fadt)(FADT*, void*);
	bool(*parse_mcfg)(MCFG*, void*);
	bool(*parse_madt)(MADT*, void*);
	bool(*parse_apic_pla)(ProcessorLocalAPIC*, void*);
	bool(*parse_apic_ia)(IOAPIC*, void*);
	bool(*parse_apic_iso)(InterruptSourceOverride*, void*);
	bool(*parse_apic_nmis)(NonMaskableInterruptSource*, void*);
} ACPI_Parser;

void acpi_init();
void acpi_shutdown();
void acpi_parse_rsdt(ACPI_Parser* parser, void* context);

#endif /* __ACPI_H__ */
