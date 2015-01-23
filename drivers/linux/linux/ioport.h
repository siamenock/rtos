#ifndef __LINUX_IOPORT_H__
#define __LINUX_IOPORT_H__

struct resource {
};

extern struct resource ioport_resource;
extern struct resource iomem_resource;

struct resource* __request_region(struct resource*, resource_size_t start, resource_size_t n, const char* name, int flags);

#define request_mem_region(start,n,name) __request_region(&iomem_resource, (start), (n), (name), 0)
#define request_mem_region_exclusive(start,n,name) __request_region(&iomem_resource, (start), (n), (name), IORESOURCE_EXCLUSIVE)

void __release_region(struct resource*, resource_size_t, resource_size_t);
#define release_mem_region(start,n)     __release_region(&iomem_resource, (start), (n))

#endif /* __LINUX_IOPORT_H__ */

