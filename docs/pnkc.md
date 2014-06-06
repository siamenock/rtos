# PacketNgin Kernel Container format

PNKC header(8 bytes)	: 0x0f, 0x0d, 0x0a, 0x02, 'P', 'N', 'K', 'C'

Kernel header
	.text (8 bytes)	: offset(uint32_t), size(uint32_t)
	.rodata(8 bytes): offset(uint32_t), size(uint32_t)
	.data(8 bytes)	: offset(uint32_t), size(uint32_t)
	.bss(8 bytes)	: offset(uint32_t), size(uint32_t)

file list
	module count	: uint32_t
	module info[]	: type(uint32_t), size(uint32_t)

Kernel body
	.text
	.rodata
	.data

Files
	
# File types
	1: system symbol map
	2: module

# Reference
.text, .rodata's offset is relative from 0xffffffff80200000
.data, .bss's offset is relative from 0xffffffff80400000
