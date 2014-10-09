#ifndef __DRIVER_PORT_H__
#define __DRIVER_PORT_H__

#include "pci.h"

DeviceType port_device_type;
Driver port_device_driver;
bool port_device_probe(PCI_Device* pci, char** name, void** data);

#endif /* __DRIVER_PORT_H__ */
