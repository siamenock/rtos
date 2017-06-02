#include "entry.h"
#include "shared.h"

#define STACK_SIZE	    512
#define CORE_COUNT	    16

	.text

	.code64
	.globl startup_64
startup_64:
	leaq	_text(%rip), %rbp
	subq	$_text - __START_KERNEL_map, %rbp

	mov	$entry - __START_KERNEL_map, %r14
	addq	%rbp, %r14

	// Boot parameter passed by Linux
	movq	%rsi, %rsi
	// Physical offset
	movq	%rbp, %rdi

	cli

	// Stack setting
	mov	$0x01, %rax
	cpuid
	shrd	$24, %ebx, %ebx
	and	$0x000000ff, %ebx
	mov	%rbp, %rsp
	addq	$stack_top  - __START_KERNEL_map, %rsp
repeat:
	cmp	$0, %ebx
	je	next

	sub	$STACK_SIZE, %rsp

	sub	$1, %ebx
	jmp	repeat
next:
	xorl	%eax, %eax
	movl	%eax, %ds
	movl	%eax, %ss
	movl	%eax, %es

	// GDTR address
	leaq	gdt(%rip), %rax

	// GDT address (GDTR address + 0x8)
	movq	%rax, %rbx
	addq	$0x8, %rbx

	// Set GDT address in GDTR
	movl	%ebx, 0x2(%rax)
	lgdt	(%rax)

	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %ss
	movl	%eax, %es

	pushq	$0		# fake return address to stop unwinder
	pushq	$0x08		# set correct cs
	pushq	%r14		# target address in negative space
	lretq

	/* Temporary stack */
stack_bottom:
	.fill   STACK_SIZE * CORE_COUNT, 1, 0   # Maximum 16 cores
stack_top:

	/* Temporary GDT */
	.section ".data"
	.balign 16
gdt:
	.word	gdt_end - gdt
	.long	0xffffffff		# GDT address. It will be modified in runtime
	.word	0
	.quad	0x0000000000000000	/* NULL descriptor */
	.quad	0x00af9a000000ffff	/* __KERNEL_CS */
	.quad	0x00cf92000000ffff	/* __KERNEL_DS */
	.quad	0x0080890000000000	/* TS descriptor */
	.quad   0x0000000000000000	/* TS continued */
gdt_end:

//TODO: fix Shared area space
	.section ".rodata"
	.balign 16
	.globl SHARED_ADDR
SHARED_ADDR:
	/* @see shared.h */
	.fill 16, 1, 0	/* mp_processors[MP_MAX_CORE_COUNT] */
	.byte 0 /* sync */
	.quad 0 /* icc_pool */
	.byte 0 /* icc_lock_alloc */
	.byte 0 /* icc_lock_free */
	.quad 0 /* icc_queues */
	.quad SHARED_MAGIC /* magic */

	/*
	 * Description table area
	 * */
	.section ".desc_table", "w", @nobits
	.balign 4096
	.globl GDTR_ADDR
GDTR_ADDR:
	.fill 0x10, 1, 0

	.globl GDT_ADDR
GDT_ADDR:
	.fill 8 /* SD8 */ * 5 + 16 /* SD16 */ * 16 /* MP_MAX_CORE_COUNT */, 1, 0
	.globl GDT_END_ADDR
GDT_END_ADDR:

	.globl TSS_ADDR
TSS_ADDR:
	.fill 104 * 16 /* MP_MAX_CORE_COUNT */

	.globl IDTR_ADDR
IDTR_ADDR:
	.fill 16, 1, 0

	.globl IDT_ADDR
IDT_ADDR:
	.fill 16 * 100
	.globl IDT_END_ADDR
IDT_END_ADDR:
	.org GDTR_ADDR + 0x100000, 0

	/*
	 * Shared area
	 * */
	/*
	 *.section ".shared", "a", @nobits
	 *.balign 4096
	 *.fill 0x100000, 1, 0
	 */

	/*
	 * Kernel data area
	 * */
	// VGA buffer
	.section ".vga_buffer","a",@nobits
	.balign 4096
	.fill 0x10000, 1, 0

	// User interrupt stack
	.section ".user_intr_stack","a",@nobits
	.balign 4096
	.fill 0x8000, 1, 0

	// Kernel interrupt stack
	.section ".kernel_intr_stack","a",@nobits
	.balign 4096
	.fill 0x8000, 1, 0

	// Kernel stack
	.section ".kernel_stack","a",@nobits
	.balign 4096
	.fill 0x10000, 1, 0

	// Page table
	.section ".pgtable","a",@nobits
	.balign 4096
	.fill 64 * 4096, 1, 0
