#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "device.h"
#include "module.h"
#include "pci.h"
#include "driver/pci.h"
#include "driver/nic.h"
#include "driver/charin.h"
#include "driver/charout.h"

#define MAX_DEVICE_COUNT	256

static Device devices[MAX_DEVICE_COUNT];

Device* device_register(DeviceType type, void* driver, void* device, void* data) {
	int init() {
		switch(type) {
			case DEVICE_TYPE_NULL:
				// Do nothing
				return 0;
			case DEVICE_TYPE_CHAR_IN:
				if(!((CharIn*)driver)->init)
					return -1;
				return ((CharIn*)driver)->init(device, data);
			case DEVICE_TYPE_CHAR_OUT:
				if(!((CharOut*)driver)->init)
					return -1;
				return ((CharOut*)driver)->init(device, data);
			case DEVICE_TYPE_NIC:
				if(!((NIC*)driver)->init)
					return -1;
				return ((NIC*)driver)->init(device, data);
			default:
				return -1;
		}
	}
	
	for(int i = 0; i < MAX_DEVICE_COUNT; i++) {
		if(devices[i].type == DEVICE_TYPE_NULL) {
			int id = init();
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
	
	void destroy() {
		switch(device->type) {
			case DEVICE_TYPE_NULL:
				// Do nothing
				break;
			case DEVICE_TYPE_NIC:
				if(((NIC*)device->driver)->destroy)
					((NIC*)device->driver)->destroy(device->id);
				break;
			case DEVICE_TYPE_CHAR_IN:
				if(((CharIn*)device->driver)->destroy)
					((CharIn*)device->driver)->destroy(device->id);
				break;
			case DEVICE_TYPE_CHAR_OUT:
				if(((CharOut*)device->driver)->destroy)
					((CharOut*)device->driver)->destroy(device->id);
				break;
			case DEVICE_TYPE_FILESYSTEM:
				// TODO: Implment it
				break;
		}
	}
	
	if(device) {
		destroy();
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
		
		PCI_ID* pci_ids = module_find(modules[i], "pci_ids", MODULE_TYPE_DATA);
		if(pci_ids) {
			count += pci_probe(*type, pci_ids, driver);
		}
	}
	
	return count;
}
