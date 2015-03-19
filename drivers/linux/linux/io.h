#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__

#include <linux/types.h>

#define mmiowb()	asm volatile("" : : : "memory")

volatile void* ioremap_nocache(resource_size_t offset, unsigned long size);
volatile void* ioremap_cache(resource_size_t offset, unsigned long size);
volatile void* ioremap_prot(resource_size_t offset, unsigned long size, unsigned long prot_val);
void iounmap(volatile void* addr);
u8 readb(void* addr);
u16 readw(void* addr);
u32 readl(void* addr);
void writeb(u8 v, void* addr);
void writew(u16 v, void* addr);
void writel(u32 v, void* addr);

#endif /* __LINUX_IO_H__ */

