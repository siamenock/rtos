#include <linux/device.h>

// PacketNgin doesn't have class concept
struct class* class_create(struct module *owner, const char *name) { return NULL; }
void class_destroy(struct class *cls) {}
int class_register(struct class *cls) { return 0; }
void class_unregister(struct class *class) {}

// PacketNgin doesn't make device explicitly
struct device *device_create(struct class *cls, 
	struct device *parent, dev_t devt, void *drvdata, 
	const char *fmt, ...) {
    return NULL;
}
void device_destroy(struct class *cls, dev_t devt) {}	
