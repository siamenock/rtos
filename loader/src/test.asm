[org 0x00]
[bits 16]

section .text
jmp	0x1000:start

SECTOR:	dw	0x0000
TOTAL equ	1024

start:
	mov	ax, cs
	mov	ds, ax
	mov	ax, 0xb800
	mov	es, ax

	%assign	i 0
	%rep	TOTAL
		%assign	i	i + 1

		mov	ax, 2
		mul	word [SECTOR]
		mov	si, ax

		mov	byte[es: si + (160 * 2)], '0' + (i % 10)
		add	word[SECTOR], 1

		%if i == TOTAL
			jmp $
		%else
			jmp (0x1000 + i * 0x20): 0x0000
		%endif

		times (512 - ($ - $$) % 512) db 0x00
	%endrep
