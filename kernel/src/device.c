#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "device.h"
#include "module.h"
#include "pci.h"
#include "driver/pci.h"

#define MAX_DEVICE_COUNT	256

static Device devices[MAX_DEVICE_COUNT];

Device* device_register(DeviceType type, void* driver, void* device, void* data) {
	for(int i = 0; i < MAX_DEVICE_COUNT; i++) {
		if(devices[i].type == DEVICE_TYPE_NULL) {
			int id = ((Driver*)driver)->init(device, data);
			if(id < 0) {
				errno = id;	// Cannot initialize the device
				return NULL;
			}
			
			devices[i].type = type;
			devices[i].driver = driver;
			devices[i].device = device;
			devices[i].id = id;
			
			return &devices[i];
		}
	}
	
	errno = 2;	// There is no space to register the device
	
	return NULL;
}

bool device_deregister(int fd) {
	Device* device = &devices[fd];
	
	if(device) {
		((Driver*)device->driver)->destroy(device->id);
		bzero(device, sizeof(Device));
		
		return true;
	} else {
		return false;
	}
}

int device_count(DeviceType type) {
	int count = 0;
	
	for(int i = 0; i < MAX_DEVICE_COUNT; i++)
		if(devices[i].type == type)
			count++;
	
	return count;
}

Device* device_get(DeviceType type, int idx) {
	int count = 0;
	
	for(int i = 0; i < MAX_DEVICE_COUNT; i++)
		if(devices[i].type == type && count++ == idx)
			return &devices[i];
	
	return NULL;
}

int device_module_init() {
	int count = 0;
	for(int i = 0; i < module_count; i++) {
		DeviceType* type = module_find(modules[i], "device_type", MODULE_TYPE_DATA);
		if(!type)
			continue;
		
		void* driver = module_find(modules[i], "device_driver", MODULE_TYPE_DATA);
		
		PCI_DEVICE_PROBE probe = module_find(modules[i], "pci_device_probe", MODULE_TYPE_FUNC);
		if(probe) {
			count += pci_probe(*type, probe, driver);
		}
	}
	
	return count;
}
