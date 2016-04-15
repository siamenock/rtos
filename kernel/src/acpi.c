#include <stdbool.h>
#include <string.h>
#include <timer.h>
#include "cpu.h"
#include "mp.h"
#include "port.h"
#include "pci.h"
#include "acpi.h"

// Ref: http://www.acpi.info/DOWNLOADS/ACPIspec50.pdf

// Root system description pointer
	
static RSDP* find_RSDP() {
	bool is_RSDP(uint8_t* p) {
		if(memcmp(p, "RSD PTR", 7) == 0) {
			uint8_t checksum = 0;
			for(size_t i = 0; i < sizeof(RSDP); i++)
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
static RSDT* rsdt;
static FADT* fadt;
static MCFG* mcfg;
static MADT* madt;
static uint16_t slp_typa;
static uint16_t slp_typb;

void acpi_parse_rsdt(ACPI_Parser* parser, void* context) {
	if(rsdp && parser->parse_rsdp && !parser->parse_rsdp(rsdp, context))
		return;

	if(rsdt && parser->parse_rsdt && !parser->parse_rsdt(rsdt, context))
		return;

	if(mcfg && parser->parse_mcfg && !parser->parse_mcfg(mcfg, context))
		return;

	if(fadt && parser->parse_fadt && !parser->parse_fadt(fadt, context))
		return;

	if(madt && parser->parse_madt && !parser->parse_madt(madt, context))
		return;

	if(madt) {
		int length = madt->length - sizeof(MADT);
		uint8_t* entry = madt->entry;

		for(int i = 0; i < length;) {
			switch(*(entry + i)) {
				case PROCESSOR_LOCAL_APIC:
					;
					ProcessorLocalAPIC* local_apic = (ProcessorLocalAPIC*)(entry + i);
					if(parser->parse_apic_pla && !parser->parse_apic_pla(local_apic, context))
						return;

					i += local_apic->record_length;
					break;
				case IO_APIC:
					;
					IOAPIC* io_apic = (IOAPIC*)(entry + i);
					if(parser->parse_apic_ia && !parser->parse_apic_ia(io_apic, context))
						return;

					i += io_apic->record_length;
					break;
				case INTERRUPT_SOURCE_OVERRIDE:
					;
					InterruptSourceOverride* interrupt_src_over = (InterruptSourceOverride*)(entry + i);
					if(parser->parse_apic_iso && !parser->parse_apic_iso(interrupt_src_over, context))
						return;

					i += interrupt_src_over->record_length;
					break;
				case NONMASKABLE_INTERRUPT_SOURCE:
					;
					NonMaskableInterruptSource* nonmask_intr_src = (NonMaskableInterruptSource*)(entry + i);
					if(parser->parse_apic_nmis && !parser->parse_apic_nmis(nonmask_intr_src, context))
						return;

					i += nonmask_intr_src->record_length;
					break;
				default://unknown type
					i += *(entry + i + 1);
					break;
			}
		}
	}
}

void acpi_init() {
	uint8_t apic_id = mp_apic_id();
	
	rsdp = find_RSDP();
	if(!rsdp) {
		if(apic_id == 0) {
			//printf("\tCannot find root system description pointer\n");
			while(1) asm("hlt");
		}
	}
	
	rsdt = (void*)(uint64_t)rsdp->address;
	
	// Find FADT and MCFG
	for(size_t i = 0; i < rsdt->length / 4; i++) {
		void* p = (void*)(uint64_t)rsdt->table[i];
		
		if(fadt == NULL && memcmp(p, "FACP", 4) == 0) {
			fadt = p;
		} else if(mcfg == NULL && memcmp(p, "MCFG", 4) == 0) {
			mcfg = p;
		} else if(madt == NULL && memcmp(p, "APIC", 4) == 0) { //fix here
			madt = p;
		} else if(fadt != NULL && mcfg != NULL && madt != NULL) {
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

	// APIC
	if(madt) {
		int length = madt->length - sizeof(MADT);
		uint8_t* entry = madt->entry;

		for(int i = 0; i < length;) {
			switch(*(entry + i)) {
				case PROCESSOR_LOCAL_APIC:
					;
					ProcessorLocalAPIC* local_apic = (ProcessorLocalAPIC*)(entry + i);
					i += local_apic->record_length;
					if(local_apic->flags) {
						mp_cores[local_apic->apic_id] = 1;
					}
					break;
				case IO_APIC:
					;
					IOAPIC* io_apic = (IOAPIC*)(entry + i);
					i += io_apic->record_length;
					break;
				case INTERRUPT_SOURCE_OVERRIDE:
					;
					InterruptSourceOverride* interrupt_src_over = (InterruptSourceOverride*)(entry + i);
					i += interrupt_src_over->record_length;
					break;
				default://unknown type
					i += *(entry + i + 1);
					break;
			}
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
		
		timer_mwait(10);
	}
	
	if(fadt->pm1b_control_block) {
		for(int i = 0; i < 300; i++) {
			if(port_in16(fadt->pm1a_control_block) == 1)
				break;
			
			timer_mwait(10);
		}
	}
}

void acpi_shutdown() {
	acpi_enable();
	
	port_out16(fadt->pm1a_control_block, slp_typa | (1 << 13));
	if(fadt->pm1b_control_block)
		port_out16(fadt->pm1b_control_block, slp_typb | (1 << 13));
}
