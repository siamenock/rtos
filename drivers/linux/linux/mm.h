#ifndef __LINUX_MM_H__
#define __LINUX_MM_H__

#include <linux/kernel.h>
#include <linux/mm_types.h>

#define PAGE_SHIFT		12
#define PAGE_SIZE 		(1ul << PAGE_SHIFT)
#define PAGE_ALIGN(addr)	ALIGN(addr, PAGE_SIZE)

void *page_address(const struct page *page);

#endif /* __LINUX_MM_H__ */
