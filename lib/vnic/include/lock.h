#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @file
 * Locking between threads.
 */

/**
 * Initialize the lock object.
 *
 * @param lock lock object
 */
void lock_init(uint8_t volatile* lock);

/**
 * Lock it.
 *
 * @param lock lock object
 */
void lock_lock(uint8_t volatile* lock);

/**
 * Try to lock it if available.
 *
 * @param lock lock object
 *
 * @return true when succeed to lock
 */
bool lock_trylock(uint8_t volatile* lock);

/**
 * Unlock it.
 *
 * @param lock lock object
 */
void lock_unlock(uint8_t volatile* lock);

#endif /* __LOCK_H__ */
