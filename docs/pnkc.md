# PacketNgin Kernel Container format

PNKC header (8 bytes)		: 'P', 'N', 'K', 'C',  0x0f, 0x0d, 0x0a, 0x02
.text offset, size (8 bytes)	: uint32_t, uint32_t
.rodata offset, size (8 bytes)	: uint32_t, uint32_t
.data offset, size (8 bytes)	: uint32_t, uint32_t
.bss offset, size (8 bytes)	: uint32_t, uint32_t
.text (variable)		: binary
.rodata (variable)		: binary
.data (variable)		: binary

.text and .rodata's offset is relative from the address of 0xffffffff80200000
.data and .bss's offset is relative from the address of 0xffffffff80400000

