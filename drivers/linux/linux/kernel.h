#ifndef __LINUX_KERNEL_H__
#define __LINUX_KERNEL_H__

#include <linux/spinlock.h>
#include <asm/byteorder.h>

#define USHRT_MAX       ((u16)(~0U))
#define SHRT_MAX        ((s16)(USHRT_MAX>>1))
#define SHRT_MIN        ((s16)(-SHRT_MAX - 1))
#define INT_MAX         ((int)(~0U>>1))
#define INT_MIN         (-INT_MAX - 1)
#define UINT_MAX        (~0U)
#define LONG_MAX        ((long)(~0UL>>1))
#define LONG_MIN        (-LONG_MAX - 1)
#define ULONG_MAX       (~0UL)
#define LLONG_MAX       ((long long)(~0ULL>>1))
#define LLONG_MIN       (-LLONG_MAX - 1)
#define ULLONG_MAX      (~0ULL)
#undef	SIZE_MAX
#define SIZE_MAX        (~(size_t)0)

#define U8_MAX          ((u8)~0U)
#define S8_MAX          ((s8)(U8_MAX>>1))
#define S8_MIN          ((s8)(-S8_MAX - 1))
#define U16_MAX         ((u16)~0U)
#define S16_MAX         ((s16)(U16_MAX>>1))
#define S16_MIN         ((s16)(-S16_MAX - 1))
#define U32_MAX         ((u32)~0U)
#define S32_MAX         ((s32)(U32_MAX>>1))
#define S32_MIN         ((s32)(-S32_MAX - 1))
#define U64_MAX         ((u64)~0ULL)
#define S64_MAX         ((s64)(U64_MAX>>1))
#define S64_MIN         ((s64)(-S64_MAX - 1))

#define ALIGN(x, a)			__ALIGN_KERNEL((x), (a))
#define __ALIGN_KERNEL(x, a)            __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_MASK(x, mask)		__ALIGN_KERNEL_MASK((x), (mask))
#define __ALIGN_KERNEL_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define PTR_ALIGN(p, a)			((typeof(p))ALIGN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

//#define cpu_to_le17(v)	(v)
//#define cpu_to_le32(v)	(v)
//#define cpu_to_le64(v)	(v)
//#define le16_to_cpu(v) 	(v)
//#define le32_to_cpu(v)	(v)
//#define le64_to_cpu(v)	(v)

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x, y) ({                             \
         typeof(x) _min1 = (x);                  \
         typeof(y) _min2 = (y);                  \
         (void) (&_min1 == &_min2);              \
         _min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({                             \
         typeof(x) _max1 = (x);                  \
         typeof(y) _max2 = (y);                  \
         (void) (&_max1 == &_max2);              \
	 _max1 > _max2 ? _max1 : _max2; })

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1: __max2; })

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define dev_err(dev, format, ...) //fprintf(stderr, format, ##__VA_ARGS__)
#define dev_warn(dev, format, ...) //fprintf(stderr, format, ##__VA_ARGS__)

#define uninitialized_var(x)	x = x

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#endif /* __LINUX_KERNEL_H__ */


