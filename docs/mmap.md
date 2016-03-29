# Step 1: Grub to BSP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000	: core.img(grub)
64KB		: loader.bin (entry, loader, GDT, stacks for 16 cores)
1MB		: modules (kernel.bin, initrd.img)

# Step 2: BSP to AP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000	: core.img(grub)
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
-1MB ~ -2MB	: Description table, mapped to 1MB ~ 2MB
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

-2MB ~ -4MB	: mapped to 2MB ~ 4MB
	+256KB?	: Kernel (PNKC, .text)
	+25KB?	: Kernel (.rodata)
	+32KB?	: Kernel.smap (8 bytes aligned)
	+2KB?	: multiboot2 tags (8 bytes aligned)
	+n?	: modules (8 bytes aligned)

-4MB + -2MB * apicid: mappped to 4MB + 2MB * apicid + +2MB
	+200KB?	: Kernel (.data, .bss)
	...	: Local malloc
	64KB	: VGA buffer
	32KB	: User interrupt stack
	32KB	: Kernel interrupt stack
	64KB	: Kernel stack
	256KB	: TLB

-4MB + -2MB * 16: initial RAM disk, mapped to 36MB ~ +sizeof initrd.img
