#include "entry.h"

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

	// FIXME: avoid to waste of memory. It's just for PNKC
	.org	0x200, 0

	.section ".data"
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

	/*
	 * Space for page tables (not in .bss so not zeroed)
	 **/
	.section ".pgtable","a",@nobits
	.balign 4096

pgtable:
	.fill 64 * 4096, 1, 0
