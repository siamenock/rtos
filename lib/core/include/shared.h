#ifndef __SHARED_H__
#define __SHARED_H__

/**
 * @file
 * Set or get shared pointer between threads.
 */

/**
 * Set shared pointer
 *
 * @param ptr the pointer from global memory area
 *
 * @see gmalloc.h
 */
void shared_set(void* ptr);

/**
 * Get shared pointer
 *
 * @return shared pointer or NULL if it's not setted yet
 */
void* shared_get();

#endif /* __SHARED_H__ */
