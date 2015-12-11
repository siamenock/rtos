#ifndef __LINUX_ATOMIC_H__
#define __LINUX_ATOMIC_H__

#include <linux/types.h>

long long atomic64_read(const atomic64_t *v);
void atomic64_set(atomic64_t *v, long long i);

void atomic64_add(long long a, atomic64_t *v);
void atomic64_sub(long long a, atomic64_t *v);
void atomic_add(long long a, atomic_t *v);
long long atomic64_add_return(long long a, atomic64_t *v);
long long atomic64_sub_return(long long a, atomic64_t *v);

#define atomic_inc(v)			atomic_add(1, (v))
#define atomic64_add_negative(a, v)	(atomic64_add_return((a), (v)) < 0)
#define atomic64_inc(v)			atomic64_add(1LL, (v))
#define atomic64_inc_return(v)		atomic64_add_return(1LL, (v))
#define atomic64_inc_and_test(v)	(atomic64_inc_return(v) == 0)
#define atomic64_sub_and_test(a, v)	(atomic64_sub_return((a), (v)) == 0)
#define atomic64_dec(v)			atomic64_sub(1LL, (v))
#define atomic64_dec_return(v)		atomic64_sub_return(1LL, (v))
#define atomic64_dec_and_test(v)	(atomic64_dec_return((v)) == 0)
#define atomic64_inc_not_zero(v)	atomic64_add_unless((v), 1LL, 0LL)

#endif /* __LINUX_ATOMIC_H__ */

