#include <stdio.h>
#include <string.h>
#define __USE_MISC
#include <unistd.h>
#include <syscall.h>
#include "lock.h"
#include "shared.h"
#include "mp.h"
#include "mmap.h"

// Ref: http://www.cs.cmu.edu/~410/doc/intel-mp.pdf
// Ref: http://www.intel.com/design/pentium/datashts/24201606.pdf
// Ref: http://www.intel.com/content/dam/doc/specification-update/64-architecture-x2apic-specification.pdf
// Ref: http://www.bioscentral.com/misc/cmosmap.htm
// Ref: http://www.singlix.com/trdos/UNIX_V1/xv6/lapic.c

int cpu_start;
int cpu_end;

static uint8_t apic_id;		// APIC ID
static uint8_t core_id;
static uint8_t core_count;

#define __NR_multikernel_boot 312
static void wakeup_ap(long kernel_start_address) {
	if(!cpu_end)
		return;

	printf("\tBooting APs: ");
	for(int cpu = cpu_start; cpu < cpu_end + 1; cpu++) {
		int apicid = syscall(__NR_multikernel_boot, cpu, KERNEL_TEXT_AREA_START);
		if(apicid < 0) {
			continue;
		}
		shared->mp_cores[apicid] = cpu;
		printf("APIC ID: %d CPU ID: %d ", apicid, cpu);
	}
	printf("Done\n");
}

void mp_init0() {
	apic_id = 0;
}

void mp_init(unsigned long kernel_start_address) {
	core_id = 0;

	shared->mp_cores[apic_id] = core_id;
// 	wakeup_ap(kernel_start_address);
}

uint8_t mp_apic_id() {
	return apic_id;
}

uint8_t mp_core_id() {
	return core_id;
}

uint8_t mp_apic_id_to_core_id(uint8_t apic_id) {
	return shared->mp_cores[apic_id];
}

uint8_t mp_core_count() {
	return core_count;
}

void mp_sync0() {
	uint32_t map = 1 << apic_id;
	
	uint32_t full = 0;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(shared->mp_cores[i] != MP_CORE_INVALID)
			full |= 1 << i;
	}

	lock_lock(&shared->sync_lock);
	shared->sync_map |= map;
	lock_unlock(&shared->sync_lock);

	while(shared->sync_map != full && shared->sync_map & map) {
		full = 0;
		for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
			if(shared->mp_cores[i] != MP_CORE_INVALID)
				full |= 1 << i;
		}
		__asm volatile("nop");
	}
}

void mp_sync() {
	uint32_t map = 1 << apic_id;
	
	uint32_t full = 0;
	for(int i = 0; i < MP_MAX_CORE_COUNT; i++) {
		if(shared->mp_cores[i] != MP_CORE_INVALID)
			full |= 1 << i;
	}

	lock_lock(&shared->sync_lock);
	if(shared->sync_map == full) {	// The first one
		shared->sync_map = map;
	} else {
		shared->sync_map |= map;
	}
	lock_unlock(&shared->sync_lock);

	while(shared->sync_map != full && shared->sync_map & map)
		__asm volatile("nop");
}

/*
 *void mp_parse_fps(MP_Parser* parser, void* context) {
 *        MP_FloatingPointerStructure* find_FloatingPointerStructure() {
 *                bool is_FloatingPointerStructure(uint8_t* p) {
 *                        if(memcmp(p, "_MP_", 4) == 0) {
 *                                uint8_t checksum = 0;
 *                                uint8_t length = *(uint8_t*)(p + 8) * 16;
 *                                for(int i = 0; i < length; i++)
 *                                        checksum += p[i];
 *                                
 *                                if(checksum == 0) {
 *                                        return true;
 *                                }
 *                        }
 *                        
 *                        return false;
 *                }
 *                
 *                // Find extended BIOS area first
 *                uint64_t addr = *(uint16_t*)0x040e * 16;
 *                for(uint8_t* p = (uint8_t*)addr; p <= (uint8_t*)(addr + 1024); p++) {
 *                        if(is_FloatingPointerStructure(p))
 *                                return (MP_FloatingPointerStructure*)p;
 *                }
 *
 *                // Find system memory area next
 *                addr = *(uint16_t*)0x0413 * 1024;
 *                for(uint8_t* p = (uint8_t*)(addr - 1024); p <= (uint8_t*)addr; p++) {
 *                        if(is_FloatingPointerStructure(p))
 *                                return (MP_FloatingPointerStructure*)p;
 *                }
 *                
 *                // Find BIOS ROM area last
 *                for(uint8_t* p = (uint8_t*)0x0f0000; p < (uint8_t*)0x0fffff; p++) {
 *                        if(is_FloatingPointerStructure(p))
 *                                return (MP_FloatingPointerStructure*)p;
 *                }
 *                
 *                return NULL;
 *        }
 *        
 *        // Read MP information
 *        static MP_FloatingPointerStructure* fps;
 *        if(!fps)
 *                fps = find_FloatingPointerStructure();
 *        
 *        if(!fps || (parser->parse_fps && !parser->parse_fps(fps, context)))
 *                return;
 *        
 *        MP_ConfigurationTableHeader* cth = (MP_ConfigurationTableHeader*)(uint64_t)fps->physical_address_pointer;
 *        if(!cth || (parser->parse_cth && !parser->parse_cth(cth, context)))
 *                return;
 *        
 *        uint8_t* type = (uint8_t*)(uint64_t)fps->physical_address_pointer + sizeof(MP_ConfigurationTableHeader);
 *        for(int i = 0; i < cth->entry_count; i++) {
 *                switch(*type) {
 *                        case 0:
 *                                if(parser->parse_pe && !parser->parse_pe((MP_ProcessorEntry*)type, context))
 *                                        return;
 *                                
 *                                type += sizeof(MP_ProcessorEntry);
 *                                break;
 *                        case 1:
 *                                if(parser->parse_be && !parser->parse_be((MP_BusEntry*)type, context))
 *                                        return;
 *                                
 *                                type += sizeof(MP_BusEntry);
 *                                break;
 *                        case 2:
 *                                if(parser->parse_iae && !parser->parse_iae((MP_IOAPICEntry*)type, context))
 *                                        return;
 *                                
 *                                type += sizeof(MP_IOAPICEntry);
 *                                break;
 *                        case 3:
 *                                if(parser->parse_iie && !parser->parse_iie((MP_IOInterruptEntry*)type, context))
 *                                        return;
 *                                
 *                                type += sizeof(MP_IOInterruptEntry);
 *                                break;
 *                        case 4:
 *                                if(parser->parse_lie && !parser->parse_lie((MP_LocalInterruptEntry*)type, context))
 *                                        return;
 *                                
 *                                type += sizeof(MP_LocalInterruptEntry);
 *                                break;
 *                        case 128:
 *                                if(parser->parse_sase && !parser->parse_sase((MP_SystemAddressSpaceEntry*)type, context))
 *                                        return;
 *                                
 *                                type += type[1] * 16;
 *                                break;
 *                        case 129:
 *                                if(parser->parse_bhde && !parser->parse_bhde((MP_BusHierarchyDescriptorEntry*)type, context))
 *                                        return;
 *                                
 *                                type += type[1] * 16;
 *                                break;
 *                        case 130:
 *                                if(parser->parse_cbasme && !parser->parse_cbasme((MP_CompatabilityBusAddressSpaceModifierEntry*)type, context))
 *                                        return;
 *                                
 *                                type += type[1] * 16;
 *                                break;
 *                        default:
 *                                // Extended types
 *                                type += type[1] * 16;
 *                }
 *        }
 *}
 *
 */
uint8_t* mp_core_map() {

	return shared->mp_cores;
}
