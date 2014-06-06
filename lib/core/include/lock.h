#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>
#include <stdbool.h>

void lock_init(uint8_t volatile* lock);
void lock_lock(uint8_t volatile* lock);
bool lock_trylock(uint8_t volatile* lock);
void lock_unlock(uint8_t volatile* lock);

#endif /* __LOCK_H__ */
