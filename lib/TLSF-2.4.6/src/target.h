#ifndef _TARGET_H_
#define _TARGET_H_

#include <stdint.h>
#include <stdbool.h>

extern void lock_init(uint8_t volatile* lock);
extern void lock_lock(uint8_t volatile* lock);
extern bool lock_trylock(uint8_t volatile* lock);
extern void lock_unlock(uint8_t volatile* lock);

#define TLSF_MLOCK_T		volatile uint8_t
#define TLSF_CREATE_LOCK(lock)	lock_init(lock)
#define TLSF_DESTROY_LOCK(lock)	do{}while(0)
#define TLSF_ACQUIRE_LOCK(lock)	lock_lock(lock)
#define TLSF_RELEASE_LOCK(lock)	lock_unlock(lock)
/*
void bzero(void*, uint32_t);
void memcpy(void*, void*, uint32_t);

#define memset(dest, src, size)	bzero(dest, size)
*/

#endif
