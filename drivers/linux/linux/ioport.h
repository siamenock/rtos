#ifndef __LINUX_IOPORT_H__
#define __LINUX_IOPORT_H__

#include <linux/types.h> 

#define IORESOURCE_BITS			0x000000ff	/* Bus-specific bits */

#define IORESOURCE_TYPE_BITS	0x00001f00	/* Resource type */
#define IORESOURCE_IO			0x00000100	/* PCI/ISA I/O ports */
#define IORESOURCE_MEM			0x00000200
#define IORESOURCE_REG			0x00000300	/* Register offsets */
#define IORESOURCE_IRQ			0x00000400
#define IORESOURCE_DMA			0x00000800
#define IORESOURCE_BUS			0x00001000

#define IORESOURCE_PREFETCH		0x00002000	/* No side effects */
#define IORESOURCE_READONLY		0x00004000
#define IORESOURCE_CACHEABLE	0x00008000
#define IORESOURCE_RANGELENGTH	0x00010000
#define IORESOURCE_SHADOWABLE	0x00020000

#define IORESOURCE_SIZEALIGN	0x00040000	/* size indicates alignment */
#define IORESOURCE_STARTALIGN	0x00080000	/* start field is alignment */

#define IORESOURCE_MEM_64		0x00100000
#define IORESOURCE_WINDOW		0x00200000	/* forwarded by bridge */
#define IORESOURCE_MUXED		0x00400000	/* Resource is software muxed */

#define IORESOURCE_EXCLUSIVE	0x08000000	/* Userland may not map this resource */
#define IORESOURCE_DISABLED		0x10000000
#define IORESOURCE_UNSET		0x20000000	/* No address assigned yet */
#define IORESOURCE_AUTO			0x40000000
#define IORESOURCE_BUSY			0x80000000	/* Driver has marked this resource busy */

#define request_mem_region(start,n,name) __request_region(&iomem_resource, (start), (n), (name), 0)
#define request_mem_region_exclusive(start,n,name) __request_region(&iomem_resource, (start), (n), (name), IORESOURCE_EXCLUSIVE)
#define release_mem_region(start,n)     __release_region(&iomem_resource, (start), (n))

struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
	struct resource *parent, *sibling, *child;
};

extern struct resource ioport_resource;
extern struct resource iomem_resource;

struct resource* __request_region(struct resource*, resource_size_t start, resource_size_t n, const char* name, int flags);
void __release_region(struct resource*, resource_size_t, resource_size_t);

#endif /* __LINUX_IOPORT_H__ */

