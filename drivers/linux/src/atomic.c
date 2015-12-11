#include <asm/atomic.h>
#include <linux/compiler.h>

// arch/x86/include/asm/alternative.h
#define LOCK_PREFIX "" 

// arch/x86/include/asm/cmpxchg.h
#define __X86_CASE_B	1
#define __X86_CASE_W	2
#define __X86_CASE_L	4
#define __X86_CASE_Q	8

#define __xchg_op(ptr, arg, op, lock)					\
	({								\
	        __typeof__ (*(ptr)) __ret = (arg);			\
		switch (sizeof(*(ptr))) {				\
		case __X86_CASE_B:					\
			asm volatile (lock #op "b %b0, %1\n"		\
				      : "+q" (__ret), "+m" (*(ptr))	\
				      : : "memory", "cc");		\
			break;						\
		case __X86_CASE_W:					\
			asm volatile (lock #op "w %w0, %1\n"		\
				      : "+r" (__ret), "+m" (*(ptr))	\
				      : : "memory", "cc");		\
			break;						\
		case __X86_CASE_L:					\
			asm volatile (lock #op "l %0, %1\n"		\
				      : "+r" (__ret), "+m" (*(ptr))	\
				      : : "memory", "cc");		\
			break;						\
		case __X86_CASE_Q:					\
			asm volatile (lock #op "q %q0, %1\n"		\
				      : "+r" (__ret), "+m" (*(ptr))	\
				      : : "memory", "cc");		\
			break;						\
		}							\
		__ret;							\
	})
void atomic_dec(atomic_t *v)
{
	asm volatile(LOCK_PREFIX "decl %0"
		     : "+m" (v->counter));
}

long long atomic64_read(const atomic64_t *v)
{
	return ACCESS_ONCE((v)->counter);
}

void atomic64_set(atomic64_t *v, long long i)
{
	v->counter = i;
}

void atomic_add(long long i, atomic_t *v)
{
	asm volatile(LOCK_PREFIX "addq %1,%0"
		     : "=m" (v->counter)
		     : "er" (i), "m" (v->counter));
}

void atomic64_add(long long i, atomic64_t *v)
{
	asm volatile(LOCK_PREFIX "addq %1,%0"
		     : "=m" (v->counter)
		     : "er" (i), "m" (v->counter));
}

void atomic64_sub(long long i, atomic64_t *v)
{
	asm volatile(LOCK_PREFIX "subq %1,%0"
		     : "=m" (v->counter)
		     : "er" (i), "m" (v->counter));
}

long long atomic64_add_return(long long i, atomic64_t *v)
{
	return i + __xchg_op(&v->counter, i, xadd, LOCK_PREFIX); // xadd(&v->counter, i);
}

long long atomic64_sub_return(long long i, atomic64_t *v)
{
	return atomic64_add_return(-i, v);
}


