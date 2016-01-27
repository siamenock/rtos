[BITS 64]

SECTION .text

global lgdt, ltr, lidt
global sti, cli, pushfq
global rdtsc, hlt;, cmpxchg
global clflush, wbinvd
global pause
global read_cr0
global read_cr2, write_cr2
global read_cr3, write_cr3
global refresh_cr3
global read_rsp, read_rbp
global read_xmms0
global mwait, monitor
global read_msr, write_msr

lgdt:
	lgdt	[rdi]
	ret

ltr:
	ltr	di
	ret

lidt:
	lidt	[rdi]
	ret

sti:
	sti
	ret

cli:
	cli
	ret

pushfq:
	pushfq
	pop	rax
	ret

rdtsc:
	push	rdx
	rdtsc
	shl	rdx, 32
	or	rax, rdx
	pop	rdx
	ret

hlt:
	hlt
	ret

;cmpxchg:
;	mov		rax, rsi
;	lock cmpxchg	byte[rdi], dl
;	je .cmpxchg_succeed
;
;	mov		rax, 0
;	ret
;
;.cmpxchg_succeed:
;	mov		rax, 1
;	ret

clflush:
	clflush	[rdi]
	ret

wbinvd:
	wbinvd
	ret

;pause:
;	pause
;	ret

read_cr0:
	mov	rax, cr0
	ret

read_cr2:
	mov	rax, cr2
	ret

write_cr2:
	mov	cr2, rdi
	ret

read_cr3:
	mov	rax, cr3
	ret

write_cr3:
	mov	cr3, rdi
	ret

refresh_cr3:
	mov	rax, cr3
	mov	cr3, rax
	ret

read_rsp:
	mov	rax, rsp
	ret

read_rbp:
	mov	rax, rbp
	ret

read_xmms0:
	movaps	[rdi], xmm0
	ret

read_msr:
	mov	ecx, edi
	rdmsr
	shl	rdx, 32
	or	rax, rdx
	ret

write_msr:
	mov	ecx, edi
	mov	eax, esi
	shr	rsi, 32
	mov	edx, esi
	wrmsr
	ret

monitor:
	mov	rax, rdi
	mov	ecx, 0
	mov	edx, 0
	monitor	rax, ecx, edx
	ret

mwait:
	mov	ecx, edi
	mov	eax, esi
	mwait	eax, ecx
	ret
