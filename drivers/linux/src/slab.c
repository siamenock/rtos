#include <linux/slab.h>
#include <string.h>
#include <gmalloc.h>

// PacketNgin use unseparted kernel space memory 
void* kmalloc(size_t size, gfp_t flags) {
	return gmalloc(size);	
}

void kfree(const void* objp) {
	gfree((void*)objp);
}

void* kzalloc(size_t size, gfp_t flags)
{
	void* ptr = gmalloc(size);
	bzero(ptr, size);
	
	return ptr;
}

void* kmalloc_node(size_t size, int flag, int node) {
	return gmalloc(size);
}
