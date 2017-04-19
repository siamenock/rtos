#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include <stdint.h>
#include <byteswap.h>
#include <malloc.h>

typedef uint8_t		u8_t;
typedef uint16_t	u16_t;
typedef uint32_t	u32_t;
typedef int8_t		s8_t;
typedef int16_t		s16_t;
typedef int32_t		s32_t;
typedef uint64_t	mem_ptr_t;

#define LWIP_ERR_T	int

#define U16_F		"u"
#define S16_F		"d"
#define X16_F		"x"
#define U32_F		"u"
#define S32_F		"d"
#define X32_F		"lx"
#define SZT_F		"u"

#define BYTE_ORDER	LITTLE_ENDIAN

#define LWIP_PLATFORM_BYTESWAP	1
#define LWIP_PLATFORM_HTONS(x)	bswap_16((x))
#define LWIP_PLATFORM_HTONL(x)	bswap_32((x))

#define LWIP_CHKSUM_ALGORITHM	2

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT	__attribute__((packed))
#define PACK_STRUCT_END
#define ALIGNED(n)		__attribute__((aligned(n)))

#ifdef LWIP_DEBUG
	#include <stdio.h>
	#define LWIP_PLATFORM_DIAG(vars)	printf vars
	#define LWIP_PLATFORM_ASSERT(flag)	printf(flag)
#else
	#define LWIP_PLATFORM_DIAG(msg)
	#define LWIP_PLATFORM_ASSERT(flag)	while (1) asm("hlt");
#endif

#endif /* __ARCH_CC_H__ */
