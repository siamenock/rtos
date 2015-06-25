[BITS 32]

global mode_switch

SECTION .text

mode_switch:
	; PAE activate
	mov	eax, cr4
	or	eax, 0x620	; OSXMMEXCPT=1, OSFXSR=1, PAE=1
	mov	cr4, eax

	; PML4 table activate
	mov	eax, edx	; edx=core_id
	mov	ebx, 0x200000
	mul	ebx
	add	eax, 0x5c0000
	mov	cr3, eax

	; IA-32e mode activate
	mov	ecx, 0xc0000080	; Read IA32_EFER register
	rdmsr
	or	eax, 0x0901	; NXE=1, LME=1, SCE=1
	wrmsr

	; Activate caching and paging
	mov	eax, cr0
	or	eax, 0xe000000e	; PG=1, CD=1, NW=1, TS=1, EM=1, MP=1
	xor	eax, 0x60000004	;       CD=1, NW=1,       EM=1
	mov	cr0, eax	; PG=1, CD=0, NW=0, TS=1, EM=0, MP=1

	jmp	0x08:0x200000	; Jump to kernel64

