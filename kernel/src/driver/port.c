#include "device.h"
#include "port.h"

int port_init(void* device, void* data) {
	return 0;
}

void port_destroy(int id) {
}

DeviceType port_device_type = DEVICE_TYPE_PORT;

Driver port_device_driver = {
	.init = port_init,
	.destroy = port_destroy
};

bool port_device_probe(PCI_Device* pci, char** name, void** data) {
	return false;
}
