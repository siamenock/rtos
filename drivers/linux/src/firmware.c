#include <linux/firmware.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <gmalloc.h>
#include "driver/fs.h"

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device) {
	char file_name[FILE_MAX_NAME_LEN];
	strcpy(file_name, "/boot/");
	strcpy(&file_name[6], name);

	int fd = open(file_name, "r");
	if(fd < 0) {
		printf("Unknown firmware: '%s'\n", file_name);
		return -1;
	}

	uint32_t size = file_size(fd);
	if(size < 0) {
		printf("\tCannot get state of file: %s\n", file_name);
		return -2;
	}
		
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
