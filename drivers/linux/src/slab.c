#include <linux/slab.h>
#include <string.h>
#include <gmalloc.h>

#define PAGE_SHIFT 8
// PacketNgin use unseparted kernel space memory 
void* kmalloc(size_t size, gfp_t flags) {
	uint64_t align;

	if(size <= 0)
		return NULL;

	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if(!p)
		return NULL;

	/* Address of the aligned memory according to the align parameter*/
	ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
	/* store the address of the malloc() above
	 * at the beginning of our total memory area.
	 * You can also use *((void **)ptr-1) = p
	 * instead of the one below.
	 */
	*((void**)((uint64_t)ptr - sizeof(void*))) = p;
	/* Return the address of aligned memory */

	return ptr; 
}

void kfree(const void* objp) {
	void* ptr = *((void**)((uint64_t)objp - sizeof(void*)));

	if(ptr)
		gfree(ptr);
}

void* kzalloc(size_t size, gfp_t flags) {
	uint64_t align; 
	if(size <= 0)
		return NULL;

	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if(!p)
		return NULL;
	bzero(p, size + align - 1 + sizeof(void*));

	/* Address of the aligned memory according to the align parameter*/
	ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
	/* store the address of the malloc() above
	 * at the beginning of our total memory area.
	 * You can also use *((void **)ptr-1) = p
	 * instead of the one below.
	 */
	*((void**)((uint64_t)ptr - sizeof(void*))) = p;
	/* Return the address of aligned memory */

	return ptr; 
}

void* kcalloc(size_t n, size_t size, gfp_t flags) {
	uint64_t align; 

	if(size <= 0)
		return NULL;

	if(n <= 0)
		return NULL;

	size *= n;
	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if(!p)
		return NULL;
	bzero(p, size + align - 1 + sizeof(void*));

	/* Address of the aligned memory according to the align parameter*/
	ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
	/* store the address of the malloc() above
	 * at the beginning of our total memory area.
	 * You can also use *((void **)ptr-1) = p
	 * instead of the one below.
	 */
	*((void**)((uint64_t)ptr - sizeof(void*))) = p;
	/* Return the address of aligned memory */
	bzero(ptr, size);

	return ptr;
}

void* kmalloc_node(size_t size, int flag, int node) {
	uint64_t align;

	if(size <= 0)
		return NULL;

	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if(!p)
		return NULL;

	/* Address of the aligned memory according to the align parameter*/
	ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
	/* store the address of the malloc() above
	 * at the beginning of our total memory area.
	 * You can also use *((void **)ptr-1) = p
	 * instead of the one below.
	 */
	*((void**)((uint64_t)ptr - sizeof(void*))) = p;
	/* Return the address of aligned memory */

	return ptr; 
}

void* vzalloc(size_t size) {
	uint64_t align; 
	if(size <= 0)
		return NULL;

	/* Check the size is whether to be square of 2 or not */
	if((size & (size - 1)) != 0)
		align = 1 << PAGE_SHIFT; // default is 4kb alignment 	
	else
		align = size;

	void *ptr;
	void *p = gmalloc(size + align - 1 + sizeof(void*));

	if(!p)
		return NULL;
	bzero(p, size + align - 1 + sizeof(void*));

	/* Address of the aligned memory according to the align parameter*/
	ptr = (void*) (((uint64_t)p + sizeof(void*) + align -1) & ~(align-1));
	/* store the address of the malloc() above
	 * at the beginning of our total memory area.
	 * You can also use *((void **)ptr-1) = p
	 * instead of the one below.
	 */
	*((void**)((uint64_t)ptr - sizeof(void*))) = p;
	/* Return the address of aligned memory */

	return ptr; 
}

void vfree(const void* objp) {
	void* ptr = *((void**)((uint64_t)objp - sizeof(void*)));

	if(ptr)
		gfree(ptr);
}

