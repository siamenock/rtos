#include <linux/firmware.h>
#include <linux/printk.h>
#include <gmalloc.h>
#include "driver/fs.h"

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device) {
	int fd = open(name, "r");
	if(fd < 0) {
		printf("Unknown firmware: '%s'\n", name);
		return -1;
	}

	Stat state;
	if(stat(fd, &state) != 0) {
		printf("\tCannot get state of file: %s\n", name);
		return -2;
	}
		
	uint32_t size = state.size;
	struct firmware* f = gmalloc(sizeof(struct firmware));
	void* data = gmalloc(size);

	if(read(fd, data, size) != size) {
		printf("Read failed\n");
		close(fd);

		return -1;
	}

	f->size = size;
	f->data = data;
	*(struct firmware**)fw = f;

	close(fd);
	
	return 0;
}

void release_firmware(const struct firmware *fw) {
	if(fw)
		gfree((struct firmware*)fw);
}
