#ifndef __LINUX_COMPILER_H__
#define __LINUX_COMPILER_H__

#define __iomem

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x)) // include/linux/compiler.h
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#endif /* __LINUX_COMPILER_H__ */
