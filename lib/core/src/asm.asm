[BITS 64]

SECTION .text

global cmpxchg

cmpxchg:
	mov		rax, rsi
	lock cmpxchg	byte[rdi], dl
	je .cmpxchg_succeed

	mov		rax, 0
	ret

.cmpxchg_succeed:
	mov		rax, 1
	ret
