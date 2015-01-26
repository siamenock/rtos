#ifndef __LINUX_INTERRUPT_H__
#define __LINUX_INTERRUPT_H__

#include <linux/irqreturn.h>

typedef irqreturn_t(*irq_handler_t)(int, void *);

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev);
void free_irq(unsigned int, void *);

#endif /* __LINUX_INTERRUPT_H__ */

