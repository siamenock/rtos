#ifndef __LINUX_IRQRETURN_H__
#define __LINUX_IRQRETURN_H__

enum irqreturn {
	IRQ_NONE	= (0 << 0),
	IRQ_HANDLED	= (1 << 0),
	IRQ_WAKE_THREAD	= (1 << 1),
};

typedef enum irqreturn irqreturn_t;

#endif /* __LINUX_IRQRETURN_H__ */

