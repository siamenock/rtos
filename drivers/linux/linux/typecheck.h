#ifndef __LINUX_TYPECHECK_H__
#define __LINUX_TYPECHECK_H__

#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

#define typecheck_fn(type,function) \
({	typeof(type) __tmp = function; \
	(void)__tmp; \
})

#endif /* __LINUX_TYPECHECK_H__ */

