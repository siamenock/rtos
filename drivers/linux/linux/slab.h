#ifndef __LINUX_SLAB_H__
#define __LINUX_SLAB_H__

void* kmalloc(size_t size, gfp_t flags);
void kfree(const void*);

#endif /* __LINUX_SLAB_H__ */

