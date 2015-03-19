#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <linux/pci.h>
#include <linux/types.h>

struct firmware {
	size_t		size;
	const u8*	data;
	void**		pages;
};

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device);
void release_firmware(const struct firmware *fw);

#endif
