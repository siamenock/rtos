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

unsigned long virt_to_phys(volatile void* address);
void* phys_to_virt(phys_addr_t address);

u8 ioread8(const volatile void *addr);
u16 ioread16(const volatile void *addr);
u32 ioread32(const volatile void *addr);

void iowrite8(u8 value, volatile void *addr);
void iowrite16(u16 value, volatile void *addr);
void iowrite32(u32 value, volatile void *addr);

#endif /* __LINUX_IO_H__ */

