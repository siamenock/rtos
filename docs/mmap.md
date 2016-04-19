# Step 1: BSP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000	: core.img(grub)
64KB		: loader.bin (entry, loader, GDT, stacks for 16 cores)
1MB		: modules (kernel.bin, initrd.img)

# Step 2: BSP, AP booting
0x07c00 ~ +512B	: boot.img(grub)
0x070000	: core.img(grub)
64KB		: loader.bin (entry, loader, GDT, stacks for 16 cores)
1MB		: Grub module area
	+315K?	: kernel.bin (grub module)
	+1M	: initrd.img (grub module)

2MB		: Kernel text area
	+256KB?	: Kernel (PNKC, .text)
	+25KB?	: Kernel (.rodata)
	+32KB?	: Kernel.smap (8 bytes aligned)
	+2KB?	: multiboot2 tags (8 bytes aligned)
	...	:

2MB + 2MB * core: Kernel data area
	+200KB?	: Kernel (.data, .bss)
	...	: 
	64KB	: Kernel stack
	256KB	: TLB

# Step 3: 64bit kernel booting
Negative address means 0xffffffff80000000 prefix.
e.g.	-1MB -> 0xffffffff80100000
	-2MB -> 0xffffffff80200000

-0MB ~ -1MB	: BIOS area, mapped to 0MB ~ 1MB

-1MB ~ -2MB	: Description table area, mapped to 1MB ~ 2MB
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

-2MB ~ -4MB	: Kernel text area, mapped to 2MB ~ 4MB
	+256KB?	: Kernel (PNKC, .text)
	+25KB?	: Kernel (.rodata)
	+32KB?	: Kernel.smap (8 bytes aligned)
	+2KB?	: multiboot2 tags (8 bytes aligned)
	+n?	: modules (8 bytes aligned)

-4MB + -2MB * apicid: Kernel data area, mappped to 4MB + 2MB * apicid + +2MB
	+200KB?	: Kernel (.data, .bss)
	...	: Local malloc
	64KB	: VGA buffer
	32KB	: User interrupt stack
	32KB	: Kernel interrupt stack
	64KB	: Kernel stack
	256KB	: TLB

-4MB + -2MB * 16: Initial RAM disk area, mapped to 36MB ~ +sizeof initrd.img

extras		: Block malloc (including not unused kernel data area)
		  Global malloc includes fragments from BIOS, Description table, 
		    Kernel data and Initial RAM disk area

