#include <lock.h>

void lock_init(uint8_t volatile* lock) {
	*lock = 0;
}

bool cmpxchg(uint8_t volatile* s1, uint8_t s2, uint8_t d);

void lock_lock(uint8_t volatile* lock) {
	while(!lock_trylock(lock))
		asm volatile("nop");
}

bool lock_trylock(uint8_t volatile* lock) {
	return cmpxchg(lock, 0, 1);
}

void lock_unlock(uint8_t volatile* lock) {
	*lock = 0;
}
