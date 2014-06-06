[BITS 64]

SECTION .text

extern isr_exception_handler
extern isr_interrupt_handler

%macro context_save 0
	push	rbp
	mov	rbp, rsp
	push	rax
	push	rbx
	push	rcx
	push	rdx
	push	rdi
	push	rsi
	push	r8
	push	r9
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15
	
	mov	ax, ds
	push	rax
	mov	ax, es
	push	rax
	push	fs
	push	gs
	
	mov	ax, 0x10
	mov	ds, ax
	mov	es, ax
	mov	gs, ax
	mov	fs, ax
%endmacro

%macro context_load 0
	pop	gs
	pop	fs
	pop	rax
	mov	es, ax
	pop	rax
	mov	ds, ax
	
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	rsi
	pop	rdi
	pop	rdx
	pop	rcx
	pop	rbx
	pop	rax
	pop	rbp
%endmacro

%assign n 0

; Exceptions: 32 CPU exceptions
%rep 32
	global isr_%[n]

	isr_%[n]:
		context_save
		mov	rdi, n
		call	isr_exception_handler
		context_load
		iretq
	%assign n n+1
%endrep

; Interrupts: 16 BIOS interrupts + 32 general handler
%rep 32
	global isr_%[n]

	isr_%[n]:
		context_save
		mov	rdi, n
		call	isr_interrupt_handler
		context_load
		iretq
	%assign n n+1
%endrep
