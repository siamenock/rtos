#include <linux/spinlock.h>
unsigned long _raw_spin_lock_irqsave(spinlock_t *lock) {
	return 0;
}

void spin_unlock_irqrestore(spinlock_t* lock, unsigned long flags) {}
void spin_lock_bh(spinlock_t *lock) {}
void spin_unlock_bh(spinlock_t *lock) {}
