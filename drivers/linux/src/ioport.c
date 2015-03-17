#include <linux/ioport.h>

// include/asm-generic/io.h
#ifndef IO_SPACE_LIMIT
#define IO_SPACE_LIMIT 0xffff
#endif

// kernel/resource.c
#define IORESOURCE_IO		0x00000100	/* PCI/ISA I/O ports */
#define IORESOURCE_MEM		0x00000200

struct resource ioport_resource = {
	.name	= "PCI IO",
	.start	= 0,
	.end	= IO_SPACE_LIMIT,
	.flags	= IORESOURCE_IO,
};
struct resource iomem_resource = {
	.name	= "PCI mem",
	.start	= 0,
	.end	= -1,
	.flags	= IORESOURCE_MEM,
};

// PacketNgin already know memory which each module use
struct resource * __request_region(struct resource *parent,
				   resource_size_t start, resource_size_t n,
				   const char *name, int flags)
{ return (void*)1; }
void __release_region(struct resource *parent, resource_size_t start, resource_size_t n) {}
