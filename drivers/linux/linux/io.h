#ifndef __LINUX_IO_H__
#define __LINUX_IO_H__

#include <linux/types.h>

extern volatile void* ioremap_nocache(resource_size_t offset, unsigned long size);
extern volatile void* ioremap_cache(resource_size_t offset, unsigned long size);
extern volatile void* ioremap_prot(resource_size_t offset, unsigned long size, unsigned long prot_val);

void iounmap(volatile void* addr);

#endif /* __LINUX_IO_H__ */

