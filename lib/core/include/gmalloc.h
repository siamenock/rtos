#ifndef __GMALLOC_H__
#define __GMALLOC_H__

#include <stddef.h>

/**
 * Allocate memory chunk from global area
 */
void *gmalloc(size_t size);

/**
 * Free memory chunk to global area
 */
void gfree(void *ptr);

/**
 * Allocate and clear memory chunk from global area
 */
void *gcalloc(size_t nmemb, size_t size);

/**
 * Reallocate memory chunk from global area
 */
void *grealloc(void *ptr, size_t size);

#endif /* __GMALLOC_H__ */
