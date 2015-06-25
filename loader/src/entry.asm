[org 0x00]
[bits 16]

section .text

start:
	jmp	entry

entry:
	mov	ax, 0x1000
	mov	ds, ax
	mov	es, ax
	
	; Check AP
	mov	ax, 0x0000
	mov	es, ax
	cmp	byte[is_a20_activated], 0x01
	je	.a20_activated
	mov	byte[is_a20_activated], 0x01
	
	; Activate A20 gate using BIOS
	mov	ax, 0x2401
	int	0x15
	jc	.a20_activate_system_control
	jmp	.a20_activated

.a20_activate_system_control:
	in	al, 0x92
	or	al, 0x02
	and	al, 0xfe
	out	0x92, al
	
.a20_activated:
	; Load GDT
	cli
	lgdt	[gdtr]
	
	; Change mode to protected mode
	mov	eax, 0x4000003b
	mov	cr0, eax
	jmp	dword 0x18: (protectedmode - $$ + 0x10000)

[bits 32]
protectedmode:
	mov	ax, 0x20
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	
	mov	ss, ax
	mov	esp, 0xfffe
	mov	ebp, 0xfffe

	; Jump to C loader
	jmp	dword 0x18:0x10200

align 8, db 0

	dw	0x0000			; padding for GDTR
gdtr:
	dw	gdtend - gdt - 1	; GDT size
	dd	(gdt - $$ + 0x10000)	; GDT address

gdt:
	; null descriptor
	dw	0x0000
	dw	0x0000
	db	0x00
	db	0x00
	db	0x00
	db	0x00
	
	; kernel code segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x9a	; p=1, dpl=0, code segment, execute/read
	db	0xaf	; g=1, d=0, l=1, limit 19:16
	db	0x00	; base 31:24
	
	; kernel data segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x92	; p=1, dpl=0, data segment, read/write
	db	0xaf	; g=1, d=0, l=1, limit 19:16
	db	0x00	; base 31:24
	
	; loader code segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x9a	; p=1, dpl=0, code segment, read/write
	db	0xcf	; g=1, d=1, l=0, limit 19:16
	db	0x00	; base 31:24
	 
	; loader data segment descriptor
	dw	0xffff	; limit 15:0
	dw	0x0000	; base 15:0
	db	0x00	; base 23:16
	db	0x92	; p=1, dpl=0, data segment, read/write
	db	0xcf	; g=1, d=1, l=0, limit 19:16
	db	0x00	; base 31:24
gdtend:

is_a20_activated:
	db	0x00

times 512 - ($ - $$) db 0x00
