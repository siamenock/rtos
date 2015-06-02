#ifndef __GMALLOC_H__
#define __GMALLOC_H__

#include <stddef.h>

/**
 * @file
 * Memory allocation from global memory area
 */
 
/**
 * Allocate memory chunk from global area
 *
 * @param size of memory chunk
 *
 * @return pointer of memory chunk or NULL if there is no available memory space
 */
void *gmalloc(size_t size);

/**
 * Free memory chunk to global area
 *
 * @param ptr the pointer of memory chunk
 */
void gfree(void *ptr);

/**
 * Allocate and clear memory chunk from global area
 *
 * @param nmemb number of memory blocks
 * @param size size of each memory block
 *
 * @return pointer of memory chunk or NULL if there is no available memory space
 */
void *gcalloc(size_t nmemb, size_t size);

/**
 * Reallocate memory chunk from global area
 *
 * @param ptr the pointer of memorh chunk
 * @param size new size of memory to realloc
 *
 * @return pointer of reallocated memory chunk or NULL if there is no available memory space
 */
void *grealloc(void *ptr, size_t size);

#endif /* __GMALLOC_H__ */
