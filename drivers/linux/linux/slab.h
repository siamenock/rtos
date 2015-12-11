#ifndef __LINUX_SLAB_H__
#define __LINUX_SLAB_H__

#include <linux/types.h>
#include <linux/workqueue.h>

void* kmalloc(size_t size, gfp_t flags);
void kfree(const void*);
void* kzalloc(size_t size, gfp_t flags);
void* kcalloc(size_t n, size_t size, gfp_t flags);
void* kmalloc_node(size_t size, int flag, int node);

void* vzalloc(size_t size);
void vfree(const void*);

#endif /* __LINUX_SLAB_H__ */

