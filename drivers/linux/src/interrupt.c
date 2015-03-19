#include <linux/interrupt.h>

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev) {
	return 0;
}
void free_irq(unsigned int irq, void* dev_id) {}
