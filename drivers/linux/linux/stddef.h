#ifndef __LINUX_STDDEF_H__
#define __LINUX_STDDEF_H__

//#include <uapi/linux/stddef.h>

#undef NULL
#define NULL ((void *)0)

//enum {
//	false	= 0,
//	true	= 1
//};

#undef offsetof
#ifdef __compiler_offsetof
#define offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif /* __LINUX_STDDEF_H__ */ 
