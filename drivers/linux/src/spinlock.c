#include <linux/spinlock.h>
unsigned long _raw_spin_lock_irqsave(spinlock_t *lock) {
	return 0;
}

void spin_unlock_irqrestore(spinlock_t* lock, unsigned long flags) {}
void spin_lock_bh(spinlock_t *lock) {}
void spin_unlock_bh(spinlock_t *lock) {}
void spin_lock_irq(spinlock_t *lock) {}
void spin_unlock_irq(spinlock_t *lock) {}
void spin_lock(spinlock_t *lock) {}
void spin_unlock(spinlock_t *lock) {}
