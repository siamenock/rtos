[BITS 64]

SECTION .text

global _apic_activate

_apic_activate:
	push	rax
	push	rcx
	push	rdx

	mov	rcx, 0x1b	; IA32_APIC_BASE MSR register
	rdmsr
	or	eax, 0x0800	; APIC global enabled
	wrmsr

	pop	rdx
	pop	rcx
	pop	rax
	ret
