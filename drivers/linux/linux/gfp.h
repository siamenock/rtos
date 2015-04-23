#ifndef __LINUX_GFP_H__
#define __LINUX_GFP_H__

#define __GFP_DMA		0x01u
#define __GFP_HIGHMEM		0x02u
#define __GFP_DMA32		0x04u
#define __GFP_MOVABLE		0x08u
#define __GFP_WAIT		0x10u
#define __GFP_HIGH		0x20u
#define __GFP_IO		0x40u
#define ___GFP_FS		0x80u
#define __GFP_COLD		0x100u
#define __GFP_NOWARN		0x200u
#define __GFP_REPEAT		0x400u
#define __GFP_NOFAIL		0x800u
#define __GFP_NORETRY		0x1000u
#define __GFP_MEMALLOC		0x2000u
#define __GFP_COMP		0x4000u
#define __GFP_ZERO		0x8000u
#define __GFP_NOMEMALLOC	0x10000u
#define __GFP_HARDWALL		0x20000u
#define __GFP_THISNODE		0x40000u
#define __GFP_RECLAIMABLE	0x80000u
#define __GFP_NOTRACK		0x200000u
#define __GFP_NO_KSWAPD		0x400000u
#define __GFP_OTHER_NODE	0x800000u
#define __GFP_WRITE		0x1000000u

#endif /* __LINUX_GFP_H__ */
