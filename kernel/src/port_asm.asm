[BITS 64]

SECTION .text

global port_in8, port_out8
global port_in16, port_out16
global port_in32, port_out32

port_in8:
	push	rdx

	mov	rax, 0
	mov	rdx, rdi
	in	al, dx

	pop	rdx
	ret

port_out8:
	push	rdx
	push	rax
	
	mov	rdx, rdi
	mov	rax, rsi
	out	dx, al
	
	pop	rax
	pop	rdx
	ret

port_in16:
	push	rdx

	mov	rax, 0
	mov	rdx, rdi
	in	ax, dx

	pop	rdx
	ret

port_out16:
	push	rdx
	push	rax
	
	mov	rdx, rdi
	mov	rax, rsi
	out	dx, ax
	
	pop	rax
	pop	rdx
	ret

port_in32:
	push	rdx

	mov	rax, 0
	mov	rdx, rdi
	in	eax, dx

	pop	rdx
	ret

port_out32:
	push	rdx
	push	rax
	
	mov	rdx, rdi
	mov	rax, rsi
	out	dx, eax
	
	pop	rax
	pop	rdx
	ret
