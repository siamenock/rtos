ENTRY(start)

STARTUP(obj/entry.o)

SECTIONS {
	.entry 0x10000 :
	{
		*(.entry)
	}
	
	.text :
	{
		*(.text)
	}
	
	.rodata ALIGN(8):
	{
		*(.rodata)
	}
	
	.data :
	{
		*(.data)
	}
	
	.bss :
	{
		*(.bss)
	}
}
