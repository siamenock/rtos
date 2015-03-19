#ifndef __LINUX_TYPES_H__
#define __LINUX_TYPES_H__

#include <asm-generic/posix_types.h>

#define __aligned_u64 __u64 __attribute__((aligned(8)))
#define __aligned_be64 __be64 __attribute__((aligned(8)))
#define __aligned_le64 __le64 __attribute__((aligned(8)))

typedef unsigned long 	kernel_ulong_t;
typedef unsigned long	u64;
typedef unsigned int	u32;
typedef unsigned short	u16;
typedef unsigned char	u8;

typedef unsigned long	__u64;
typedef unsigned int	__u32;
typedef unsigned short	__u16;
typedef unsigned char	__u8;

typedef unsigned long	__be64;
typedef unsigned int	__be32;
typedef unsigned short	__be16;
typedef unsigned char	__be8;

typedef unsigned long	__le64;
typedef unsigned int	__le32;
typedef unsigned short	__le16;
typedef unsigned char	__le8;

typedef u64		uint64_t;
typedef u32		uint32_t;
typedef u16		uint16_t;
typedef u8		uint8_t;

typedef __u32		__kernel_dev_t;
typedef __kernel_dev_t	dev_t;
typedef unsigned short	umode_t; 

typedef __kernel_size_t	size_t;
typedef __kernel_time_t	time_t;

typedef u64			dma_addr_t;
typedef u64			phys_addr_t;  
typedef phys_addr_t	resource_size_t;

typedef struct {  
	long counter;
} atomic_t;
 
typedef struct {  
	long counter;
} atomic64_t;

typedef enum {
	GFP_KERNEL,
	GFP_ATOMIC,
} gfp_t;


#endif /* __LINUX_TYPES_H__ */

