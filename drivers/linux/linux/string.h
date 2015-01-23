#ifndef __LINUX_STRING_H__
#define __LINUX_STRING_H__

#include <linux/types.h>

void* memset(void *, int, size_t);
void* memcpy(void *,const void *,__kernel_size_t);

#endif /* __LINUX_STRING_H__ */

