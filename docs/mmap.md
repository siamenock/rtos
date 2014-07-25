# System memory map
0x0000000000000000 - 0x000000000009f400: Memory(1)
0x000000000009f400 - 0x00000000000a0000: Reserved(1)
0x00000000000f0000 - 0x0000000000100000: Reserved(1)
0x0000000000100000 - 0x000000000fffe000: Memory(1)
0x000000000fffe000 - 0x0000000010000000: Reserved(1)
0x00000000fffc0000 - 0x0000000100000000: Reserved(1)

          fec00000 Local APIC
          fee00000 IO APIC
          fec00000

# Kernel virtual memory
ffffffff80200000
Offset           : 0
Table            : 1
Directory        : 1fe
Directory Pointer: 1ff
Signature        : 7f

ffffffff80400000
Offset           : 0
Table            : 2
Directory        : 1fe
Directory Pointer: 1ff
Signature        : 7f

# Original Physical memory map
0x07c0 ~ +512B	: boot
64KB ~		: loader (3.5KB)
1MB ~ +264K	: TLB (264K)
	4K	: L2 Page Directory
	4K	: L3 Page Directory
	256K	: L4 Page Table
1MB + 264K ~ 	: GDT (1976B)
	16B	: GDTR
	40B	: Segment Descriptor
	256B	: TSS Descriptor (16 cores)
	1664B	: TSS (16 cores)
1MB + 264K + 1976B ~ : IDT (1616B)
	16B	: IDTR
	1600B	: IDT
2MB ~		: kernel code (45KB)
6MB ~ 7MB	: Kernel stack (64KB * 16 cores)
7MB ~ 8MB	: TS (64KB * 16 cores);
16MB ~ 32MB	: malloc

# Physical memory map
0x07c0 ~ +512B	: boot
64KB ~		: loader (3.5KB)

- Global: 0MB ~ 4MB
0MB ~ 640KB	: Global malloc

1MB ~ 2MB	: Description table
 		: GDT (1976B)
	16B	: GDTR
	40B	: Segment Descriptor
	256B	: TSS Descriptor (16 cores)
	1664B	: TSS (16 cores)
	
		: IDT (1616B)
	16B	: IDTR
	1600B	: IDT
	
	extra	: Global malloc
	
	64KB	: Shared

2MB ~ 4MB	: kernel global
	nB	: Kernel code
	nB	: Kernel rodata
	nB	: module code/data[]
	extra	: Global malloc

- Local: 4 + 2nMB + 6 + 2nMB
4MB ~ 6MB	: Kernel local
	nKB	: Kernel data and bss
	extra	: Local malloc
	32KB	: User interrupt stack
	32KB	: Kernel interrupt stack
	64KB	: Kernel stack
	256KB	: TLB

- User: 6 + 2nMB ~

# Kernel virtual memory map

Global: 0MB ~ 4MB	->	-2GB ~ -2GB + 4MB
Local: (4+2nMB ~ 6+2nMB)->	-2GB + 4MB ~ -2GB + 6MB
User: 6+2nMB		->	-2GB + 6MB ~

