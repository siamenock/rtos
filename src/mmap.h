#ifndef __MMAP_H__
#define __MMAP_H__

/* This header describes PacketNgin memory map. (docs/mmap.md) */

#include <stdint.h>
#include "mp.h"

#define R_BASE                      0x200000

#define VIRTUAL_TO_PHYSICAL(addr)	(~0xffffffff80000000L & (addr))
#define PHYSICAL_TO_VIRTUAL(addr)	(0xffffffff80000000L | (addr))

/*
 * BIOS area (0M ~ 1M)
 **/
#define BIOS_AREA_START             0x0
#define BIOS_AREA_END               0x100000                /* 1 MB */
#define IVT_AREA_START              0x0
#define IVT_AREA_END                0x03FF
#define BDA_AREA_START              0x0400
#define BDA_AREA_END                0x04FF

/*
 * Description table area (1M ~ 2M)
 **/
// NOTE: This area must be accessed by virtual memory since 0 ~ 2M area is identity mapped
#define DESC_TABLE_AREA_START       (0x100000 + R_BASE)    /* 1 MB */
#define DESC_TABLE_AREA_END         (0x200000 + R_BASE)    /* 2 MB */

/*
 * Kernel text area (2M ~ 4M)
 **/
#define KERNEL_TEXT_AREA_START      (0x200000 + R_BASE)    /* 2 MB */
#define KERNEL_TEXT_AREA_SIZE       0x200000                /* 2 MB */
#define KERNEL_TEXT_AREA_END        (0x400000 + R_BASE)    /* 4 MB */

#define GDTR_ADDR	                (0x100000 + R_BASE)
#define GDTR_END_ADDR	            (GDTR_ADDR + 16)
#define GDT_ADDR	                GDTR_END_ADDR
#define GDT_END_ADDR	            (GDT_ADDR + 8 * 5 + 16 * MP_MAX_CORE_COUNT)	// SD8 * 5 + SD16 * 16
#define TSS_ADDR	                GDT_END_ADDR
#define TSS_END_ADDR	            (TSS_ADDR + 104 * MP_MAX_CORE_COUNT)

#define IDTR_ADDR	                TSS_END_ADDR
#define IDTR_END_ADDR	            (IDTR_ADDR + 16)
#define IDT_ADDR	                IDTR_END_ADDR
#define IDT_END_ADDR	            (IDT_ADDR + 16 * 100)

/*
 * Kernel data area (4M ~ 6M)
 **/
#define KERNEL_DATA_AREA_START      (0x400000 + R_BASE)    /* 4 MB */
#define KERNEL_DATA_AREA_END        (0x600000 + R_BASE)    /* 6 MB */

// End of kernel - defined in linker script
extern char __bss_end[];

#define KERNEL_DATA_START           (0x400000 + R_BASE)    /* 4 MB */
#define KERNEL_DATA_END             VIRTUAL_TO_PHYSICAL((uint64_t)__bss_end)

#define LOCAL_MALLOC_START          KERNEL_DATA_END
#define LOCAL_MALLOC_END            VGA_BUFFER_START

#define VGA_BUFFER_START            (USER_INTR_STACK_START - VGA_BUFFER_SIZE)
#define VGA_BUFFER_SIZE             0x10000                 /* 64 KB */
#define VGA_BUFFER_END              USER_INTR_STACK_START

#define USER_INTR_STACK_START       (KERNEL_INTR_STACK_START - USER_INTR_STACK_SIZE)
#define USER_INTR_STACK_SIZE        0x8000                  /* 32 KB */
#define USER_INTR_STACK_END         KERNEL_INTR_STACK_START

#define KERNEL_INTR_STACK_START     (KERNEL_STACK_START - KERNEL_INTR_STACK_SIZE)
#define KERNEL_INTR_STACK_SIZE      0x8000                  /* 32 KB */
#define KERNEL_INTR_STACK_END       KERNEL_STACK_START

#define KERNEL_STACK_START          (PAGE_TABLE_START - KERNEL_STACK_SIZE)
#define KERNEL_STACK_SIZE           0x10000                /* 64 KB */
#define KERNEL_STACK_END            PAGE_TABLE_START

#define PAGE_TABLE_START            (KERNEL_DATA_AREA_END - (4096 * 64))
#define PAGE_TABLE_END              KERNEL_DATA_AREA_END

/*
 * Kernel data area (6M ~ 36M)
 **/

// Kernel data area above (4M ~ 6M) is duplicated here for 15 cores PacketNgin support maximum 16 cores.

/*
 * Ramdisk area (36M ~ sizeof(initrd.img))
 **/
// FIXME: Ramdisk area should be shrinked by measuring runtime core count
#define RAMDISK_START               (KERNEL_TEXT_AREA_START + KERNEL_TEXT_AREA_SIZE * 16)

#endif /* __MMAP_H__ */
