#ifndef __LINUX_SOCKET_H__
#define __LINUX_SOCKET_H__

#include <asm/sockios.h>

typedef unsigned short __kernel_sa_family_t;
typedef __kernel_sa_family_t    sa_family_t;

struct sockaddr {
	sa_family_t		sa_family;		/* address family, AF_xxx       */
	unsigned char	sa_data[14];	/* 14 bytes of protocol address */
};

#endif /* __LINUX_SOCKET_H__ */

