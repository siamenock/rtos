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

# Step 1: Grub to BSP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000 	: core.img(grub)
64KB		: loader.bin (entry, loader, GDT, stacks for 16 cores)
1MB		: modules (kernel.bin, kernel.smap)

# Step 2: BSP to AP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000 	: core.img(grub)
64KB		: loader.bin (entry, loader, GDT, stacks for 16 cores)
1MB		:
	+315K?	: kernel.bin (grub module)
	+1M	: initrd.img (grub module)

2MB		: 
	+256KB?	: Kernel (PNKC, .text)
	+25KB?	: Kernel (.rodata)
	+32KB?	: Kernel.smap (8 bytes aligned)
	+2KB?	: multiboot2 tags (8 bytes aligned)
	...	:

2MB + 2MB * core:
	+200KB?	: Kernel (.data, .bss)
	...	: 
	64KB	: Kernel stack
	256KB	: TLB

# Step 3: 64bit kernel booting
1MB ~ 2MB	: Description table
 		: GDT (1976B)
	16B	: GDTR
	40B	: Segment Descriptor
	256B	: TSS Descriptor (16 cores)
	1664B	: TSS (16 cores)
	
		: IDT (1616B)
	16B	: IDTR
	1600B	: IDT
	
	...	: Global malloc
	
	nB?	: Shared

2MB		: 
	+256KB?	: Kernel (PNKC, .text)
	+25KB?	: Kernel (.rodata)
	+32KB?	: Kernel.smap (8 bytes aligned)
	+2KB?	: multiboot2 tags (8 bytes aligned)
	+n?	: modules (8 bytes aligned)

4MB + 2MB * apicid:
	+200KB?	: Kernel (.data, .bss)
	...	: Local malloc
	64KB	: VGA buffer
	32KB	: User interrupt stack
	32KB	: Kernel interrupt stack
	64KB	: Kernel stack
	256KB	: TLB

4MB + 2MB * 16	: RAM disk

