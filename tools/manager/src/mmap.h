#ifndef __MMAP_H__
#define __MMAP_H__

/*
 * This header describes PacketNgin memory map. (docs/mmap.md)
 * All of these addresses are virtual address. Address mapping is
 * described in PacketNgin linker script (kernel/build/elf_x86_64.ld)
 **/

#include <stdint.h>

uint64_t PHYSICAL_OFFSET;

/*
 * BIOS area (0M ~ 2M)
 **/
#define BIOS_AREA_START		0x0
#define BIOS_AREA_END		0x200000 /* 2 MB */

#define BDA_AREA_START		0x0400
#define BDA_AREA_END		0x04FF

/*
 * Description table area (2M ~ 3M)
 **/
char* DESC_TABLE_AREA_START;
char* DESC_TABLE_AREA_END;

char* GDTR_ADDR;
char* GDT_ADDR;
char* GDT_END_ADDR;
char* TSS_ADDR;

char* IDTR_ADDR;
char* IDT_ADDR;
char* IDT_END_ADDR;

 /*
  * Kernel text area (4M ~ 6M)
  **/
char* KERNEL_TEXT_AREA_START;
char* KERNEL_TEXT_AREA_END;
#define KERNEL_TEXT_AREA_SIZE		(KERNEL_TEXT_AREA_END - KERNEL_TEXT_AREA_START)

/*
 * Kernel data area (6M ~ 8M)
 **/
char* KERNEL_DATA_AREA_START;
char* KERNEL_DATA_AREA_END;
#define KERNEL_DATA_AREA_SIZE		(KERNEL_DATA_AREA_END - KERNEL_DATA_AREA_START)

char* KERNEL_DATA_START;
char* KERNEL_DATA_END;

char* VGA_BUFFER_START;
char* VGA_BUFFER_END;

char* USER_INTR_STACK_START;
char* USER_INTR_STACK_END;

char* KERNEL_INTR_STACK_START;
char* KERNEL_INTR_STACK_END;

char* KERNEL_STACK_START;
char* KERNEL_STACK_END;

char* PAGE_TABLE_START;
char* PAGE_TABLE_END;

char* LOCAL_MALLOC_START;
char* LOCAL_MALLOC_END;

char* SHARED_ADDR;

/*
 * Kernel data area (8M ~ 38M)
 *
 * Kernel data area above (4M ~ 6M) is duplicated here for 15 cores.
 * PacketNgin support maximum 16 cores.
 **/

/*
 * Ramdisk area (38M ~ sizeof(initrd.img))
 **/
#define RAMDISK_START               (KERNEL_TEXT_AREA_START + KERNEL_TEXT_AREA_SIZE * 16)

#endif /* __MMAP_H__ */
