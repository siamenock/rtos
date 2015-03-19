#ifndef __ASM_BUG_H__
#define __ASM_BUG_H__

#define BUG_ON(condition)	if(condition) { printf("Bug!\n"); while(1) asm("hlt"); }
#define BUG()				BUG_ON(true)

#endif /* __ASM_BUG_H__ */
