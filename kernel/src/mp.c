#include <string.h>
#include "acpi.h"
#include "page.h"
#include "lock.h"
#include "mp.h"

// Ref: http://www.cs.cmu.edu/~410/doc/intel-mp.pdf
// Ref: http://www.intel.com/design/pentium/datashts/24201606.pdf
// Ref: http://www.intel.com/content/dam/doc/specification-update/64-architecture-x2apic-specification.pdf
// Ref: http://www.bioscentral.com/misc/cmosmap.htm
// Ref: http://www.singlix.com/trdos/UNIX_V1/xv6/lapic.c

static uint8_t apic_id;		// APIC ID
static uint8_t core_id;
static uint8_t core_count;

static uint32_t sync_map;
#define SYNC_MAP	*(uint32_t volatile*)VIRTUAL_TO_PHYSICAL((uint64_t)&sync_map)
static uint8_t sync_lock;
#define SYNC_LOCK	(uint8_t volatile*)VIRTUAL_TO_PHYSICAL((uint64_t)&sync_lock)

extern uint64_t _ioapic_address;
extern uint64_t _apic_address;

static void cpuid(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
	asm volatile("cpuid"
		: "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
		: "a"(*a), "b"(*b), "c"(*c), "d"(*d));
}

static bool parse_iae(MP_IOAPICEntry* entry, void* context) {
	_ioapic_address = (uint64_t)entry->io_apic_address;
	return true;
}

void mp_init() {
	// Get APIC address
	uint32_t a, b, c = 0x1b, d;
	asm volatile("rdmsr" : "=a"(a) : "c"(c));

	_apic_address = a & 0xfffff000;

	// Get APIC ID
	a = 0x01;
	cpuid(&a, &b, &c, &d);
	apic_id = (b >> 24) & 0xff;
	// Analyze floating pointer structure
	//   Get IO APIC address
	//   Other core APIC IDs
	MP_Parser parser = {
		.parse_iae = parse_iae
	};

	mp_parse_fps(&parser, NULL);

	memset(mp_cores, 0x0, sizeof(mp_cores));

	acpi_init();

	// Calculate core ID
	core_id = 0;
	for(int i = 0; i < apic_id; i++) {
		if(mp_cores[i])
			core_id++;
	}

	// Calculate core count
	core_count = 0;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(mp_cores[i])
			mp_cores[i] = core_count++;
		else
			mp_cores[i] = MP_CORE_INVALID;
	}
}

uint8_t mp_apic_id() {
	return apic_id;
}

uint8_t mp_core_id() {
	return core_id;
}

uint8_t mp_apic_id_to_core_id(uint8_t apic_id) {
	return mp_cores[apic_id];
}

uint8_t mp_core_count() {
	return core_count;
}

void mp_sync() {
	uint32_t map = 1 << apic_id;
	
	uint32_t full = 0;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(mp_cores[i] != MP_CORE_INVALID)
			full |= 1 << i;
	}

	lock_lock(SYNC_LOCK);
	if(SYNC_MAP == full) {	// The first one
		SYNC_MAP = map;
	} else {
		SYNC_MAP |= map;
	}
	lock_unlock(SYNC_LOCK);

	while(SYNC_MAP != full && SYNC_MAP & map)
		asm volatile("nop");
}

void mp_parse_fps(MP_Parser* parser, void* context) {
	MP_FloatingPointerStructure* find_FloatingPointerStructure() {
		bool is_FloatingPointerStructure(uint8_t* p) {
			if(memcmp(p, "_MP_", 4) == 0) {
				uint8_t checksum = 0;
				uint8_t length = *(uint8_t*)(p + 8) * 16;
				for(int i = 0; i < length; i++)
					checksum += p[i];
				
				if(checksum == 0) {
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
	
	// Read MP information
	static MP_FloatingPointerStructure* fps;
	if(!fps)
		fps = find_FloatingPointerStructure();
	
	if(!fps || (parser->parse_fps && !parser->parse_fps(fps, context)))
		return;
	
	MP_ConfigurationTableHeader* cth = (MP_ConfigurationTableHeader*)(uint64_t)fps->physical_address_pointer;
	if(!cth || (parser->parse_cth && !parser->parse_cth(cth, context)))
		return;
	
	uint8_t* type = (uint8_t*)(uint64_t)fps->physical_address_pointer + sizeof(MP_ConfigurationTableHeader);
	for(int i = 0; i < cth->entry_count; i++) {
		switch(*type) {
			case 0:
				if(parser->parse_pe && !parser->parse_pe((MP_ProcessorEntry*)type, context))
					return;
				
				type += sizeof(MP_ProcessorEntry);
				break;
			case 1:
				if(parser->parse_be && !parser->parse_be((MP_BusEntry*)type, context))
					return;
				
				type += sizeof(MP_BusEntry);
				break;
			case 2:
				if(parser->parse_iae && !parser->parse_iae((MP_IOAPICEntry*)type, context))
					return;
				
				type += sizeof(MP_IOAPICEntry);
				break;
			case 3:
				if(parser->parse_iie && !parser->parse_iie((MP_IOInterruptEntry*)type, context))
					return;
				
				type += sizeof(MP_IOInterruptEntry);
				break;
			case 4:
				if(parser->parse_lie && !parser->parse_lie((MP_LocalInterruptEntry*)type, context))
					return;
				
				type += sizeof(MP_LocalInterruptEntry);
				break;
			case 128:
				if(parser->parse_sase && !parser->parse_sase((MP_SystemAddressSpaceEntry*)type, context))
					return;
				
				type += type[1] * 16;
				break;
			case 129:
				if(parser->parse_bhde && !parser->parse_bhde((MP_BusHierarchyDescriptorEntry*)type, context))
					return;
				
				type += type[1] * 16;
				break;
			case 130:
				if(parser->parse_cbasme && !parser->parse_cbasme((MP_CompatabilityBusAddressSpaceModifierEntry*)type, context))
					return;
				
				type += type[1] * 16;
				break;
			default:
				// Extended types
				type += type[1] * 16;
		}
	}
}

uint8_t* mp_core_map() {
	return mp_cores;
}
