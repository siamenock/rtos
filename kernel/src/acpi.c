#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "time.h"
#include "cpu.h"
#include "mp.h"
#include "port.h"
#include "pci.h"
#include "acpi.h"

// Ref: http://www.acpi.info/DOWNLOADS/ACPIspec50.pdf

// Root system description pointer
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
	
static RSDP* find_RSDP() {
	bool is_RSDP(uint8_t* p) {
		if(memcmp(p, "RSD PTR", 7) == 0) {
			uint8_t checksum = 0;
			for(int i = 0; i < sizeof(RSDP); i++)
				checksum += p[i];
			
			if(checksum == 0) {
				return true;
			}
		}
		
		return false;
	}
	
	for(uint8_t* p = (uint8_t*)0x0e0000; p < (uint8_t*)0x0fffff; p++) {
		if(is_RSDP(p))
			return (RSDP*)p;
	}
	
	return NULL;
}

static RSDP* rsdp;
static FADT* fadt;
static MCFG* mcfg;
static uint16_t slp_typa;
static uint16_t slp_typb;

void acpi_init() {
	uint8_t core_id = mp_core_id();
	
	rsdp = find_RSDP();
	if(!rsdp) {
		if(core_id == 0) {
			printf("\tCannot find root system description pointer\n");
			while(1) asm("hlt");
		}
	}
	
	RSDT* rsdt = (void*)(uint64_t)rsdp->address;
	
	// Find FADT and MCFG
	for(int i = 0; i < rsdt->length / 4; i++) {
		void* p = (void*)(uint64_t)rsdt->table[i];
		
		if(fadt == NULL && memcmp(p, "FACP", 4) == 0) {
			fadt = p;
		} else if(mcfg == NULL && memcmp(p, "MCFG", 4) == 0) {
			mcfg = p;
		} else if(fadt != NULL && mcfg != NULL) {
			break;
		}
	}
	
	// MCFG
	if(mcfg) {
		int length = (mcfg->length - sizeof(ACPIHeader) - 8) / sizeof(MMapConfigSpace);
		for(int i = 0; i < length; i++) {
			for(int j = mcfg->mmaps[i].start; j <= mcfg->mmaps[i].end; j++) {
				pci_mmio[j] = (void*)mcfg->mmaps[i].address;
			}
		}
	}
	
	// FACP
	uint8_t* s5_addr  = NULL;;
	if(fadt) {
		ACPIHeader* dsdt = (void*)(uint64_t)(fadt->dsdt);
		uint8_t* p = (void*)dsdt + sizeof(ACPIHeader);
		// TODO: QEMU bug?? dsdt->length is too small
		while(1) {
			if(memcmp(p, "_S5_", 4) == 0) {
				s5_addr = p;
				break;
			}
			p++;
		}
	}
	
	if(s5_addr && ((s5_addr[-1] == 0x08 || (s5_addr[-2] == 0x08 && s5_addr[-1] == '\\')) && s5_addr[4] == 0x12)) {
		s5_addr += 5;
		s5_addr += ((*s5_addr & 0xc0) >> 6) + 2;

		if(*s5_addr == 0x0a)
			s5_addr++;
		
		slp_typa = *s5_addr << 10;
		s5_addr++;
		
		if(*s5_addr == 0x0a)
			s5_addr++;
		
		slp_typb = *s5_addr << 10;
	}
}

static void acpi_enable() {
	if(port_in16(fadt->pm1a_control_block) != 0)
		return;	// Already enabled
	
	port_out8(fadt->smi_command_port, fadt->acpi_enable);
	for(int i = 0; i < 300; i++) {
		if(port_in16(fadt->pm1a_control_block) == 1)
			break;
		
		cpu_mwait(10);
	}
	
	if(fadt->pm1b_control_block) {
		for(int i = 0; i < 300; i++) {
			if(port_in16(fadt->pm1a_control_block) == 1)
				break;
			
			cpu_mwait(10);
		}
	}
}

void acpi_shutdown() {
	acpi_enable();
	
	port_out16(fadt->pm1a_control_block, slp_typa | (1 << 13));
	if(fadt->pm1b_control_block)
		port_out16(fadt->pm1b_control_block, slp_typb | (1 << 13));
}
