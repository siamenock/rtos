#ifndef __LINUX_MUTEX_H__
#define __LINUX_MUTEX_H__

#define mutex_init(lock)

struct mutex {
};

void mutex_lock(struct mutex *lock);
int mutex_trylock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);

#endif /* __LINUX_MUTEX_H__ */
