#include <linux/firmware.h>
#include <linux/printk.h>
#include <rootfs.h>
#include <gmalloc.h>

int request_firmware(const struct firmware **fw, const char *name, PCI_Device *device) {
	uint32_t size;
	void* mmap = rootfs_file(name, &size);
	if(mmap) {
		struct firmware* f = gmalloc(sizeof(struct firmware));
		f->data = mmap;
		f->size = size;
		*(struct firmware**)fw = f;
	} else {
		printf("Unknown firmware: '%s'\n", name);
		return -1;
	}
	
	return 0;
}

void release_firmware(const struct firmware *fw) {
	if(fw)
		gfree((struct firmware*)fw);
}
