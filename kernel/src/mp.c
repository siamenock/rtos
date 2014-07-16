#include <stdio.h>
#include <string.h>
#include <lock.h>
#include "port.h"
#include "asm.h"
#include "apic.h"
#include "cpu.h"
#include "shared.h"
#include "mp.h"

// Ref: http://www.cs.cmu.edu/~410/doc/intel-mp.pdf
// Ref: http://download.intel.com/design/archives/processors/pro/docs/24201606.pdf
// Ref: http://www.intel.com/content/dam/doc/specification-update/64-architecture-x2apic-specification.pdf

static MP_FloatingPointerStructure* fps;
static MP_ConfigurationTableHeader* cth;

static uint8_t core_id = 0xff;
static bool is_ht = false;
static uint8_t core_count = 0;
static uint8_t last_apic_id = 0;
static uint8_t bus_isa_id = (uint8_t)-1;
static uint8_t bus_pci_id = (uint8_t)-1;

extern uint64_t _ioapic_address;
extern uint64_t _apic_address;

void mp_init0() {
	uint32_t a, b, c, d;
	asm("cpuid"
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d)
		: "a"(1));
	
	core_id = (b >> 24) & 0xff;
	
	is_ht = !!(d & (1 << 28));
}

void mp_analyze() {
	MP_FloatingPointerStructure* find_FloatingPointerStructure() {
		bool is_FloatingPointerStructure(uint8_t* p) {
			if(memcmp(p, "_MP_", 4) == 0) {
				uint8_t checksum = 0xff;
				uint8_t length = *(uint8_t*)(p + 8) * 16;
				for(int i = 0; i < length; i++)
					if(i != 10)
						checksum += p[i];
				checksum = ~checksum;
				
				if(checksum == p[10]) {
					return true;
				}
			}
			
			return false;
		}
		
		// Find extended BIOS area first
		uint64_t addr = *(uint16_t*)0x040e * 16;
		for(uint8_t* p = (uint8_t*)addr; p <= (uint8_t*)(addr + 1024); p++) {
			if(is_FloatingPointerStructure(p))
				return (MP_FloatingPointerStructure*)p;
		}

		// Find system memory area next
		addr = *(uint16_t*)0x0413 * 1024;
		for(uint8_t* p = (uint8_t*)(addr - 1024); p <= (uint8_t*)addr; p++) {
			if(is_FloatingPointerStructure(p))
				return (MP_FloatingPointerStructure*)p;
		}
		
		// Find BIOS ROM area last
		for(uint8_t* p = (uint8_t*)0x0f0000; p < (uint8_t*)0x0fffff; p++) {
			if(is_FloatingPointerStructure(p))
				return (MP_FloatingPointerStructure*)p;
		}
		
		return NULL;
	}
	
	void process_FloatingPointerStructure() {
		cth = (MP_ConfigurationTableHeader*)(uint64_t)fps->physical_address_pointer;
	}

	void process_ConfigurationTableHeader() {
		_apic_address = (uint64_t)cth->local_apic_address;
	}
	
	void process_ProcessorEntry(MP_ProcessorEntry* entry) {
		if(entry->cpu_flags & 0x01) {	// enabled
			core_count++;
			last_apic_id = entry->local_apic_id;
		}
	}
	
	void process_BusEntry(MP_BusEntry* entry) {
		if(memcmp(entry->bus_type, "PCI", 3) == 0) {
			bus_pci_id = entry->bus_id;
		} else if(memcmp(entry->bus_type, "ISA", 3) == 0) {
			bus_isa_id = entry->bus_id;
		}
	}

	void process_IOAPICEntry(MP_IOAPICEntry* entry) {
		if(_ioapic_address) {
			if(core_id == 0)
				printf("\tWARNING: This kernel does not support multiple IO APICs!!!");
		}
		
		_ioapic_address = (uint64_t)entry->io_apic_address;
	}

	void process_IOInterruptEntry(MP_IOInterruptEntry* entry) {
	}

	void process_LocalInterruptEntry(MP_LocalInterruptEntry* entry) {
	}
	
	// Read MP information
	fps = find_FloatingPointerStructure();
	if(!fps) {
		if(core_id == 0) {
			printf("\tCannot find floating pointer structure\n");
			while(1) asm("hlt");
		}
	}
	
	process_FloatingPointerStructure();
	process_ConfigurationTableHeader();
	
	uint8_t* type = (uint8_t*)(uint64_t)fps->physical_address_pointer + sizeof(MP_ConfigurationTableHeader);
	int i;
	for(i = 0; i < cth->entry_count; i++) {
		switch(*type) {
			case 0:
				process_ProcessorEntry((MP_ProcessorEntry*)type);
				type += sizeof(MP_ProcessorEntry);
				break;
			case 1:
				process_BusEntry((MP_BusEntry*)type);
				type += sizeof(MP_BusEntry);
				break;
			case 2:
				process_IOAPICEntry((MP_IOAPICEntry*)type);
				type += sizeof(MP_IOAPICEntry);
				break;
			case 3:
				process_IOInterruptEntry((MP_IOInterruptEntry*)type);
				type += sizeof(MP_IOInterruptEntry);
				break;
			case 4:
				process_LocalInterruptEntry((MP_LocalInterruptEntry*)type);
				type += sizeof(MP_LocalInterruptEntry);
				break;
			default:
				if(core_id == 0)
					printf("\tExtended type: %d\n", *type);
				type += type[1] * 16;
		}
	}
	
	uint8_t threads_per_core = last_apic_id / (core_count - 1);
	uint8_t thread_count = threads_per_core * core_count;
	
	if(core_id == 0) {
		printf("\tIO APIC Address: %x\n", _ioapic_address);
		printf("\tLocal APIC Address: %x\n", _apic_address);
		printf("\tPhysical core count: %d\n", core_count);
		printf("\tHyper threading supported: %s\n", is_ht ? "yes" : "no");
		if(is_ht) {
			printf("\tThreads per core: %d\n", threads_per_core);
			printf("\tTotal thread count: %d\n", thread_count);
		}
	}
	
	if(is_ht)
		core_count = thread_count;
}

void mp_init() {
	printf("\tSend core init IPI to APs...\n");
	apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS | 
				APIC_TM_LEVEL | 
				APIC_LV_ASSERT | 
				APIC_DM_PHYSICAL | 
				APIC_DMODE_INIT);
	
	
	cpu_uwait(10);
	
	if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
		printf("\tCannot send core init IPI.\n");
		return;
	}
	
	apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS | 
				APIC_TM_LEVEL | 
				APIC_LV_DEASSERT | 
				APIC_DM_PHYSICAL | 
				APIC_DMODE_INIT);
	
	cpu_nwait(100);
	
	if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
		printf("\tCannot send core init IPI.\n");
		return;
	}
	
	printf("\tSend core startup IPI to APs...\n");
	/*
	for(int i = 0; i < 2; i++) {
		apic_write64(APIC_REG_ICR, APIC_DSH_OTHERS | 
					APIC_TM_EDGE | 
					APIC_LV_DEASSERT | 
					//APIC_LV_ASSERT | 
					APIC_DM_PHYSICAL | 
					APIC_DMODE_STARTUP |
					0x10);	// Startup address: 0x10 = 0x10000 / 4KB
		
		cpu_nwait(200);
		
		if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
			printf("\tCannot send core startup IPI(%d/2).\n", i);
			return;
		}
	}
	*/
	
	mp_wait_init();
	printf("\tBooting APs: ");
	for(int j = 1; j < core_count; j++) {
		for(int i = 0; i < 2; i++) {
			apic_write64(APIC_REG_ICR, ((uint64_t)j << 56) |
						APIC_DSH_NONE | 
						APIC_TM_EDGE | 
						APIC_LV_DEASSERT | 
						APIC_DM_PHYSICAL | 
						APIC_DMODE_STARTUP |
						0x10);	// Startup address: 0x10 = 0x10000 / 4KB
			
			cpu_nwait(200);
			
			if(apic_read32(APIC_REG_ICR) & APIC_DS_PENDING) {
				printf("\tCannot send core startup IPI(%d/2).\n", i);
				return;
			}
		}
		
		mp_wait(j);
		printf("\%d ", j);
	}
	printf("Done\n");
}

void mp_wait_init() {
	shared->mp_wait_map = 0;
}

void mp_wait(uint8_t core_id) {
	uint32_t map = 1 << core_id;
	
	while((shared->mp_wait_map & map) == 0)
		asm volatile("nop");
}

void mp_wakeup() {
	uint32_t map = 1 << core_id;
	
	shared->mp_wait_map |= map;
}

void mp_sync() {
	uint32_t map = 1 << core_id;
	
	uint32_t full = 0;
	for(int i = 0; i < core_count; i++)
		full |= 1 << i;
	
	lock_lock(&shared->mp_sync_lock);
	if(shared->mp_sync_map == full) {	// The first one
		shared->mp_sync_map = map;
	} else {
		shared->mp_sync_map |= map;
	}
	lock_unlock(&shared->mp_sync_lock);
	
	while(shared->mp_sync_map != full && shared->mp_sync_map & map)
		asm volatile("nop");
}

inline uint8_t mp_core_count() {
	return core_count;
}

inline uint8_t mp_core_id() {
	return core_id;
}

inline uint8_t mp_bus_isa_id() {
	return bus_isa_id;
}

inline uint8_t mp_bus_pci_id() {
	return bus_pci_id;
}

inline MP_FloatingPointerStructure* mp_FloatingPointerStructure() {
	return fps;
}

void mp_iterate_IOInterruptEntry(bool(*fn)(MP_IOInterruptEntry*)) {
	uint8_t* type = (uint8_t*)(uint64_t)fps->physical_address_pointer + sizeof(MP_ConfigurationTableHeader);
	int i;
	for(i = 0; i < cth->entry_count; i++) {
		switch(*type) {
			case 0:
				type += sizeof(MP_ProcessorEntry);
				break;
			case 1:
				type += sizeof(MP_BusEntry);
				break;
			case 2:
				type += sizeof(MP_IOAPICEntry);
				break;
			case 3:
				if(!fn((MP_IOInterruptEntry*)type))
					return;
				type += sizeof(MP_IOInterruptEntry);
				break;
			case 4:
				type += sizeof(MP_LocalInterruptEntry);
				break;
			default:
				type += type[1] * 16;
		}
	}
}
