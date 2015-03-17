#ifndef __LINUX_ERR_H__
#define __LINUX_ERR_H__

#include <asm/errno.h>
#include <stdbool.h> 

#define MAX_ERRNO	4095

#ifndef __ASSEMBLY__

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)
/* Deprecated */
#define PTR_RET(p) PTR_ERR_OR_ZERO(p)

void* ERR_PTR(long error);
long PTR_ERR(const void *ptr);
bool IS_ERR(const void *ptr);
bool IS_ERR_OR_NULL(const void *ptr);
void* ERR_CAST(const void *ptr);
int PTR_ERR_OR_ZERO(const void *ptr);
#endif

#endif /* __LINUX_ERR_H__ */
