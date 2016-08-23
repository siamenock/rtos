#ifndef __ENTRY_H__
#define __ENTRY_H__

#define __START_KERNEL_map	0xffffffff80000000
#define __START_KERNEL		0xffffffff80200000

#define __ALIGN		.align 4,0x90

#define ENTRY(name)	\
	.globl name;	\
	__ALIGN;	\
	name:

#ifndef __ASSEMBLY__
#include "bootparam.h"

#define COMMAND_LINE_SIZE   2048

extern struct boot_params boot_params;
extern char boot_command_line[COMMAND_LINE_SIZE];
extern uint64_t PHYSICAL_OFFSET;

#endif /* __ASSEMBLY__ */

#endif /* __ENTRY_H__ */
