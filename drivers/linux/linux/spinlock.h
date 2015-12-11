#ifndef __LINUX_SPINLOCK_H__
#define __LINUX_SPINLOCK_H__

#include <linux/typecheck.h>
#include <linux/u64_stats_sync.h> // included by <linux/seqlock.h> 

#define DEFINE_SPINLOCK(x)	spinlock_t x = 0;

#define spin_lock_init(_lock) 
#define raw_spin_lock_irqsave(lock, flags)	\
do {						\
	typecheck(unsigned long, flags);	\
	flags = _raw_spin_lock_irqsave(lock);	\
} while (0)

#define spin_lock_irqsave(lock, flags)		\
do {						\
	raw_spin_lock_irqsave(lock, flags);	\
} while (0)

typedef volatile unsigned char spinlock_t;

unsigned long _raw_spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t* lock, unsigned long flags);
void spin_lock_bh(spinlock_t *lock);
void spin_unlock_bh(spinlock_t *lock);
void spin_lock_irq(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif /* __LINUX_SPINLOCK_H__ */

