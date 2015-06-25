[org 0x00]
[bits 16]

section .text

jmp		0x07c0:start

TOTAL:	dw	0x00	; number of loader blocks

start:
	; Init stack: 0x0000 ~ 0xffff
	mov	ax, 0x0000
	mov	ss, ax
	mov	sp, 0xfffe
	mov	bp, 0xfffe

	; Save drive
	mov	ax, 0x07c0
	mov	ds, ax
	mov	byte [DRIVE], dl
	
	; Call e820
	mov	ax, 0x07e0
	mov	es, ax
	mov	di, 0
	mov	[es:di], ax
	
	mov	ax, 0x07e0
	mov	es, ax
	mov	di, 4
	
	call	do_e820
	
	; Init ds, es register
	mov	ax, 0xb800
	mov	es, ax
	
; Clear screen
	mov	si, 0
	
.clear:
	mov	byte [es:si], 0x00
	mov	byte [es:si + 1], 0x07
	
	add	si, 2
	cmp	si, 80 * 25 * 2
	jl	.clear

	mov	si, 0
	mov	di, 0
	
	; Print MESSAGEs
	push	MSG_LOADING	; string
	push	0		; line
	call	print
	add	sp, 6
	
	; Reset disk
	mov	ax, 0		; BIOS reset
	mov	dl, byte[DRIVE]
	int	0x13
	jc	diskerror
	
	; Read disk parameter
	mov	ah, 0x08	; Read disk parameters
	mov	dl, byte[DRIVE]
	int	0x13
	jc	diskerror
	
	mov	byte [LAST_HEAD], dh
	mov	al, cl
	and	al, 0x3f
	
	mov	byte [LAST_SECTOR], al
	mov	byte [LAST_TRACK], ch
	
	; Read sector
	mov	si, 0x1000
	mov	es, si
	mov	bx, 0x0000
	mov	di, word [TOTAL]
	
read:
	cmp	di, 0
	je	readend
	sub	di, 1
	
	mov	ah, 0x02		; BIOS Read sector
	mov	al, 0x01		; sector count
	mov	dl, byte [DRIVE]
	mov	ch, byte [TRACK]	; track
	mov	cl, byte [SECTOR]	; sector
	mov	dh, byte [HEAD]		; head
	int	0x13

	add	si, 0x20		; 512 bytes
	mov	es, si

	mov	al, byte [SECTOR]
	add	al, 1
	mov	byte [SECTOR], al
	cmp	al, byte [LAST_SECTOR]
	jle	read

	; Read next header
	add	byte [HEAD], 1
	mov	byte [SECTOR], 1
	
	mov	al, byte[LAST_HEAD]
	cmp	byte [HEAD], al
	jg	.nexttrack
	
	jmp	read
	
.nexttrack:
	; Read next track
	mov	byte [HEAD], 0
	add	byte [TRACK], 1
	jmp	read

readend:
	; Loop
	jmp	0x1000:0x0000

diskerror:
	push	MSG_ERROR_DISK
	push	1
	call	print

	jmp	$

; function(X:word, Y:word, string:word)
print:
	push	bp
	mov	bp, sp
	
	push	es
	push	si
	push	di
	push	ax
	push	cx
	push	dx
	
	mov	ax, 0xb800
	mov	es, ax
	
	; line
	mov	ax, word [bp + 4]
	mov	si, 160
	mul	si
	mov	di, ax
	
	mov	si, word [bp + 6]

.printloop:
	mov	cl, byte [si]
	cmp	cl, 0
	je	.printend
	
	mov	byte [es:di], cl
	
	add	si, 1
	add	di, 2
	jmp	.printloop

.printend:
	pop	dx
	pop	cx
	pop	ax
	pop	di
	pop	si
	pop	es
	pop	bp
	ret

; use the INT 0x15, eax= 0xE820 BIOS function to get a memory map
; inputs: es:di -> destination buffer for 24 byte entries
; outputs: bp = entry count, trashes all registers except esi
do_e820:
	xor ebx, ebx		; ebx must be 0 to start
	xor bp, bp		; keep an entry count in bp
	
	mov edx, 0x0534d4150	; Place "SMAP" into edx
	mov eax, 0xe820
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes
	int 0x15
	jc short .failed	; carry set on first call means "unsupported function"
	mov edx, 0x0534d4150	; Some BIOSes apparently trash this register?
	cmp eax, edx		; on success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx		; ebx = 0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xe820		; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes again
	int 0x15
	jc short .e820f		; carry set means "end of list already reached"
	mov edx, 0x0534d4150	; repair potentially trashed register
.jmpin:
	jcxz .skipent		; skip any 0 length entries
	cmp cl, 20		; got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1	; if so: is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8]	; get lower dword of memory region length
	or ecx, [es:di + 12]	; "or" it with upper dword to test for zero
	jz .skipent		; if length qword is 0, skip entry
	inc bp			; got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx		; if ebx resets to 0, list is complete
	jne short .e820lp
.e820f:
	mov	di, 0
	mov	[es:di], bp	; store the entry count
	clc			; there is "jc" on end of list to this point, so the carry must be cleared
	ret
.failed:
	stc			; "function unsupported" error exit
	push	MSG_ERROR_E820
	push	1
	call	print

	jmp	$

MSG_LOADING:	db 'Loading PacketNgin...', 0
MSG_ERROR_DISK:	db 'ERROR: Disk', 0
MSG_ERROR_E820:	db 'ERROR: e820', 0

SECTOR:	db	0x02
HEAD:	db	0x00
TRACK:	db	0x00

DRIVE:		db	0x00
LAST_SECTOR:	db	0x00
LAST_HEAD:	db	0x00
LAST_TRACK:	db	0x00

times	510 - ($ - $$) db 0x00
db	0x55
db	0xaa
