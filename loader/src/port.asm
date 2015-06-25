# Port I/O function for loader
[BITS 32]

SECTION .text

global port_in8, port_out8
global port_in16, port_out16
global port_in32, port_out32

port_in8:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x4
	mov	edx, DWORD [ebp + 0x8]
	in	al, dx

	leave
	ret

port_out8:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x8
	mov	edx, DWORD [ebp + 0x8]
	mov	eax, DWORD [ebp + 0xc]
	out	dx, al

	leave	
	ret

port_in16:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x4
	mov	edx, DWORD [ebp + 0x8]
	in	ax, dx

	leave
	ret

port_out16:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x8
	mov	edx, DWORD [ebp + 0x8]
	mov 	eax, DWORD [ebp + 0xc]
	out	dx, ax

	leave
	ret

port_in32:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x4
	mov	edx, DWORD [ebp + 0x8]
	in	eax, dx

	leave
	ret

port_out32:
	push	ebp
	mov	ebp, esp
	sub	esp, 0x8
	mov	edx, DWORD [ebp + 0x8]
	mov 	eax, DWORD [ebp + 0xc]
	out	dx, eax

	leave
	ret
