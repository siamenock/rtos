#ifndef __GMALLOC_H__
#define __GMALLOC_H__

#include <stddef.h>

void *gmalloc(size_t size);
void gfree(void *ptr);
void *gcalloc(size_t nmemb, size_t size);
void *grealloc(void *ptr, size_t size);

#endif /* __GMALLOC_H__ */
