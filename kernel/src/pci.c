#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <util/list.h>
#include "device.h"
#include "port.h"
#include "gmalloc.h"
#include "driver/port.h"

#include "pci.h"

#define PCI_CONFIG_ADDRESS	0xcf8
#define PCI_CONFIG_DATA		0xcfc

#define INVALID_VENDOR		0xffff

#define MMIO(bus, slot, function, reg)		(((uint64_t)bus << 20) | ((uint64_t)slot << 15) | ((uint64_t)function << 12) | ((uint64_t)reg))
#define PORTIO(bus, slot, function, reg)	((1 << 31) | (bus << 16) | (slot << 11) | (function << 8) | (reg & 0xfc))

#define PCI_DUMP		0

void* pci_mmio[PCI_MAX_BUS];

int pci_cache_line_size;

PCI_Device pci_devices[PCI_MAX_DEVICES];
int pci_devices_count;
	
static uint8_t _pci_read8(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix);
static void _pci_write8(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint8_t data, bool is_pcix);
static uint16_t _pci_read16(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix);
static void _pci_write16(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint16_t data, bool is_pcix);
static uint32_t _pci_read32(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix);
static void _pci_write32(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint32_t data, bool is_pcix);

static int pci_count() {
	int count = 0;
	
	int bus, slot, function;
	for(bus = 0; bus < PCI_MAX_BUS; bus++) {
		for(slot = 0; slot < PCI_MAX_SLOT; slot++) {
			for(function = 0; function < PCI_MAX_FUNCTION; function++) {
				uint16_t vendor_id = _pci_read16(bus, slot, function, PCI_VENDOR_ID, false);
				if(vendor_id == INVALID_VENDOR)
					goto next;
				
				uint8_t header_type = _pci_read8(bus, slot, function, PCI_HEADER_TYPE, false);
				
				if(++count > PCI_MAX_DEVICES)
					goto done;
				
				if(!(header_type & 0x80))
					break;
			}
next:
		;
		}
	}
done:
	
	return count;
}

static void pci_analyze() {
	uint32_t count = 0;
	uint32_t bus, slot, function;
	for(bus = 0; bus < PCI_MAX_BUS; bus++) {
		for(slot = 0; slot < PCI_MAX_SLOT; slot++) {
			for(function = 0; function < PCI_MAX_FUNCTION; function++) {
				uint16_t vendor_id = _pci_read16(bus, slot, function, PCI_VENDOR_ID, false);
				if(vendor_id == INVALID_VENDOR)
					goto next;
				
				uint16_t device_id = _pci_read16(bus, slot, function, PCI_DEVICE_ID, false);
				uint8_t header_type = _pci_read8(bus, slot, function, PCI_HEADER_TYPE, false);
				
				PCI_Device* device = &pci_devices[count];
				bzero(device, sizeof(PCI_Device));
				device->vendor_id = vendor_id;
				device->device_id = device_id;
				device->bus = bus;
				device->slot = slot;
				device->function = function;
				device->irq = pci_read8(device, PCI_INTERRUPT_LINE);
				
				// capabilities
				if(pci_read16(device, PCI_STATUS) & PCI_STATUS_CAP_LIST) {
					int reg = PCI_CAPABILITY_LIST;
					if(pci_read8(device, PCI_HEADER_TYPE) == PCI_HEADER_TYPE_CARDBUS)
						reg = PCI_CB_CAPABILITY_LIST;
					
					for(int i = 0; i < 48; i++) { // TTL is 48 (from Linux)
						reg = pci_read8(device, reg);
						if(reg < 0x40)
							break;
						
						reg &= ~3;
						
						uint8_t id = pci_read8(device, reg + PCI_CAP_LIST_ID);
						if(id == 0xff)
							break;
						
						if(id <= PCI_CAP_ID_MAX)
							device->caps[id] = reg;
						
						reg += PCI_CAP_LIST_NEXT;
					}
				}
				
				// PCIe capabilities
				if(device->caps[PCI_CAP_ID_EXP] && !!pci_mmio[device->bus]) {
					void* mmio = pci_mmio[bus];
					mmio +=	((uint64_t)device->bus << 20) |
						((uint64_t)device->slot << 15) |
						((uint64_t)device->function << 12);
					
					int reg = 0x100;
					PCIeExtendedCapability* cap = mmio + reg;
					while(cap->id != 0 && reg != 0) {
						device->capse[cap->id] = reg;
						
						reg = cap->next;
						cap = mmio + reg;
					}
				}
				
				device->priv = NULL;
				
				if(++count > PCI_MAX_DEVICES)
					goto done;
				
				if(!(header_type & 0x80))
					break;
			}
next:
			;
		}
	}
done:
	;
}

int pci_get_entrys(PCI_Bus_Entry* bus_entry) {
	uint32_t bus, slot, function;
	uint32_t bus_count = 0;

	for(bus = 0; bus < PCI_MAX_BUS; bus++) {
		bool bus_has_function = false;
		for(slot = 0; slot < PCI_MAX_SLOT; slot++) {
			bool slot_has_function = false;
			uint16_t slot_count = bus_entry[bus_count].slot_count;

			for(function = 0; function < PCI_MAX_FUNCTION; function++) {
				uint16_t vendor_id = _pci_read16(bus, slot, function, PCI_VENDOR_ID, false);
				if(vendor_id == INVALID_VENDOR)
					continue;

				uint16_t function_count = bus_entry[bus_count].slot_entry[slot_count].function_count;
				slot_has_function = true;

				uint16_t device_id = _pci_read16(bus, slot, function, PCI_DEVICE_ID, false);
				uint8_t header_type = _pci_read8(bus, slot, function, PCI_HEADER_TYPE, false);

				
				bus_entry[bus_count].slot_entry[slot_count].function_entry[function_count].function = function;
				bus_entry[bus_count].slot_entry[slot_count].function_count++;

			}

			if(slot_has_function) {
				bus_entry[bus_count].slot_entry[slot_count].slot = slot;
				bus_entry[bus_count].slot_count++;
				bus_has_function = true;
			}
		}

		if(bus_has_function) {
			bus_entry[bus_count].bus = bus;
			bus_count++;
		}
	}

	return bus_count;
}

void pci_data_dump(uint8_t bus, uint8_t slot, uint8_t function, uint8_t level) {
	uint16_t length  = 0;
	bool is_pcix = false;

	if(_pci_read16(bus, slot, function, PCI_STATUS, false) & PCI_STATUS_CAP_LIST) {
		int reg = PCI_CAPABILITY_LIST;
		if(_pci_read8(bus, slot, function, PCI_HEADER_TYPE, false) == PCI_HEADER_TYPE_CARDBUS)
			reg = PCI_CB_CAPABILITY_LIST;
		
		for(int i = 0; i < 48; i++) { // TTL is 48 (from Linux)
			reg = _pci_read8(bus, slot, function, reg, false);
			if(reg < 0x40)
				break;
			
			reg &= ~3;
			
			uint8_t id = _pci_read8(bus, slot, function, reg + PCI_CAP_LIST_ID, false);
			if(id == 0xff)
				break;
			
			if(id <= PCI_CAP_ID_MAX) {
				if(id == PCI_CAP_ID_EXP) {
					is_pcix = true;
					break;
				}
			}
			
			reg += PCI_CAP_LIST_NEXT;
		}
	}

	if(is_pcix) {
		switch(level) {
			case PCI_DUMP_LEVEL_X:
			case PCI_DUMP_LEVEL_XX:
				length = 64;
				break;
			case PCI_DUMP_LEVEL_XXX:
				length = 256;
				break;
			case PCI_DUMP_LEVEL_XXXX:
				length = 4096;
				break;
		}
	} else {
		switch(level) {
			case PCI_DUMP_LEVEL_X:
			case PCI_DUMP_LEVEL_XX:
				length = 64;
				break;
			case PCI_DUMP_LEVEL_XXX:
			case PCI_DUMP_LEVEL_XXXX:
				length = 256;
				break;
		}
	}

	for(int i = 0; i < length;) {
		printf("%02x: ", i);
		for(int j =0 ; j < 16; j++, i++) {
			printf("%02x ", _pci_read8(bus, slot, function, i, is_pcix));
		}
		printf("\n");
	}
}

void pci_init() {
	pci_devices_count = pci_count();

	pci_analyze();

#if PCI_DUMP
	for(int i = 0; i < pci_devices_count; i++) {
		PCI_Device* device = &pci_devices[i];
		if(!device->caps[PCI_CAP_ID_EXP])
			continue;
		
		printf("Bus: %d, Slot: %d, Function: %d ", device->bus, device->slot, device->function);
		printf("Vendor ID: %x, Device ID: %x\n", device->vendor_id, device->device_id);
		
		printf("Command: %x, Status: %x\n", pci_read16(device, PCI_COMMAND), pci_read16(device, PCI_STATUS));
		printf("Revision ID: %x, Prog IF: %x, Subclass: %x, Class code: %x\n", pci_read8(device, PCI_REVISION_ID), pci_read8(device, PCI_PROG_IF), pci_read8(device, PCI_SUB_CLASS), pci_read8(device, PCI_CLASS_CODE));
		printf("Subsystem Vendor ID: %x, Subsystem ID: %x\n", pci_read16(device, PCI_SUBSYSTEM_VENDOR_ID), pci_read16(device, PCI_SUBSYSTEM_ID));
		printf("header type: %x\n\n", pci_read8(device, PCI_HEADER_TYPE));
		printf("\tcapability[%x] = %x\n", PCI_CAP_ID_EXP, device->caps[PCI_CAP_ID_EXP]);
		printf("\t Caps: ");
		for(int i = 0; i <= PCI_CAP_ID_MAX; i++) {
			if(device->caps[i])
				printf("[%x] %x ", i, device->caps[i]);
		}
		printf("\n");
		printf("\t ExCaps: ");
		for(int i = 0; i <= PCI_CAP_ID_MAX; i++) {
			if(device->capse[i])
				printf("[%x] %x ", i, device->capse[i]);
		}
		printf("\n");
	}
#endif
	
	// Probe PCIe port
	//pci_probe(port_device_type, port_device_probe, &port_device_driver);
}

uint32_t pci_device_size(uint16_t vendor_id, uint16_t device_id) {
	uint32_t count = 0;
	
	for(int i = 0; i < pci_devices_count; i++) {
		PCI_Device* device = &pci_devices[i];
		if(device->vendor_id == vendor_id && device->device_id == device_id)
			count++;
	}
	
	return count;
}

PCI_Device* pci_device(uint16_t vendor_id, uint16_t device_id, uint32_t index) {
	for(int i = 0; i < pci_devices_count; i++) {
		PCI_Device* device = &pci_devices[i];
		if(device->vendor_id == vendor_id && device->device_id == device_id && index-- == 0)
			return device;
	}
	
	return NULL;
}

bool pci_enable(PCI_Device* device) {
	bool changed = false;
	
	uint16_t enable = PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
	uint16_t command = pci_read16(device, PCI_COMMAND);
	if((command & enable) != enable) {
		command |= enable;
		pci_write16(device, PCI_COMMAND, command);
		changed = true;
	}
	
	uint8_t latency_timer = pci_read8(device, PCI_LATENCY_TIMER);
	if(latency_timer < 32) {
		pci_write8(device, PCI_LATENCY_TIMER, 32);
		changed = true;
	}
	
	return changed;
}

bool pci_power_set(PCI_Device* device) {
	return false;
}

uint8_t pci_read8(PCI_Device* device, uint32_t reg) {
	return _pci_read8(device->bus, device->slot, device->function, reg, !!device->caps[PCI_CAP_ID_EXP]);
}

static uint8_t _pci_read8(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		
		return *(uint8_t*)mmio;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		return port_in8(PCI_CONFIG_DATA + (reg & 3));
	}
}

void pci_write8(PCI_Device* device, uint32_t reg, uint8_t data) {
	return _pci_write8(device->bus, device->slot, device->function, reg, data, !!device->caps[PCI_CAP_ID_EXP]);
}

static void _pci_write8(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint8_t data, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		
		*(uint8_t*)mmio = data;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		port_out8(PCI_CONFIG_DATA + (reg & 3), data);
	}
}

uint16_t pci_read16(PCI_Device* device, uint32_t reg) {
	return _pci_read16(device->bus, device->slot, device->function, reg, !!device->caps[PCI_CAP_ID_EXP]);
}

static uint16_t _pci_read16(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		
		return *(uint16_t*)mmio;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		return port_in16(PCI_CONFIG_DATA + (reg & 2));
	}
}

void pci_write16(PCI_Device* device, uint32_t reg, uint16_t data) {
	return _pci_write16(device->bus, device->slot, device->function, reg, data, !!device->caps[PCI_CAP_ID_EXP]);
}

static void _pci_write16(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint16_t data, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		
		*(uint16_t*)mmio = data;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		port_out16(PCI_CONFIG_DATA + (reg & 2), data);
	}
}

uint32_t pci_read32(PCI_Device* device, uint32_t reg) {
	return _pci_read32(device->bus, device->slot, device->function, reg, !!device->caps[PCI_CAP_ID_EXP]);
}

static uint32_t _pci_read32(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		
		return *(uint32_t*)mmio;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		return port_in32(PCI_CONFIG_DATA);
	}
}

void pci_write32(PCI_Device* device, uint32_t reg, uint32_t data) {
	return _pci_write32(device->bus, device->slot, device->function, reg, data, !!device->caps[PCI_CAP_ID_EXP]);
}

static void _pci_write32(uint8_t bus, uint8_t slot, uint8_t function, uint32_t reg, uint32_t data, bool is_pcix) {
	void* mmio = pci_mmio[bus];
	if(is_pcix && mmio) {
		mmio +=	MMIO(bus, slot, function, reg);
		*(uint32_t*)mmio = data;
	} else {
		port_out32(PCI_CONFIG_ADDRESS, PORTIO(bus, slot, function, reg));
		port_out32(PCI_CONFIG_DATA, data);
	}
}

uint8_t pci_pcie_ver(PCI_Device* device) {
	return pci_read8(device, device->caps[PCI_CAP_ID_EXP] + PCI_EXP_FLAGS) & 0x0f;
}

//uint8_t pci_pcie_type(PCI_Device* device) {
//	return (pci_read8(device, device->caps[PCI_CAP_ID_EXP] + PCI_EXP_FLAGS) >> 4) & 0x0f;
//}

int pci_probe(DeviceType type, PCI_DEVICE_PROBE probe, Driver* driver) {
	int count = 0;
	
	for(int i = 0; i < pci_devices_count; i++) {
		PCI_Device* pci = &pci_devices[i];
		
		// Try to probe
		void* data = NULL;
		char* name;
		if(!pci->name && probe(pci, &name, &data) && device_register(type, driver, pci, data)) {
			int len = strlen(name) + 1;
			pci->name = malloc(len);
			memcpy(pci->name, name, len);
			
			printf("\tPCI device probed: id: %d, vendor: %04x, device: %04x, name: %s\n", i, pci->vendor_id, pci->device_id, pci->name);
			
			count++;
		}
	}
	
	return count;
}
