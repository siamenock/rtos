#ifndef __LINUX_INTERRUPT_H__
#define __LINUX_INTERRUPT_H__

#include <linux/irqreturn.h>

#define IRQF_DISABLED		0x00000020
#define IRQF_SHARED		0x00000080
#define IRQF_PROBE_SHARED	0x00000100
#define __IRQF_TIMER		0x00000200
#define IRQF_PERCPU		0x00000400
#define IRQF_NOBALANCING	0x00000800
#define IRQF_IRQPOLL		0x00001000
#define IRQF_ONESHOT		0x00002000
#define IRQF_NO_SUSPEND		0x00004000
#define IRQF_FORCE_RESUME	0x00008000
#define IRQF_NO_THREAD		0x00010000
#define IRQF_EARLY_RESUME	0x00020000

typedef irqreturn_t(*irq_handler_t)(int, void *);

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev);
void free_irq(unsigned int, void *);

#endif /* __LINUX_INTERRUPT_H__ */

