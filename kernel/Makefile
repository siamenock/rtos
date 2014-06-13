.PHONY: all clean

CFLAGS = -I ../lib/TLSF/src -I ../lib/core/include -I ../lib/lwip/src/include/ -I ../lib/lwip/src/include/ipv4 -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

DIR = obj obj/driver

OBJS = obj/entry.o \
	obj/main.o obj/stdio.o obj/shell.o obj/port.o obj/asm.o obj/gdt.o obj/idt.o \
	obj/isr.o obj/device.o obj/pci.o obj/malloc.o obj/gmalloc.o obj/task.o obj/task_asm.o \
	obj/mp.o obj/apic.o obj/apic_asm.o obj/ioapic.o obj/rtc.o obj/cpu.o \
	obj/loader.o obj/event.o obj/shared.o obj/ni.o obj/manager.o obj/icc.o \
	obj/string.o obj/httpd.o obj/module.o obj/symbols.o obj/rootfs.o \
	obj/driver/linux.o obj/driver/dummy.o \
	obj/driver/vga.o obj/driver/keyboard.o obj/driver/stdin.o obj/driver/stdout.o obj/driver/bfs.o
	#obj/driver/rtl8139.o \
	#obj/driver/r1000_n.o obj/driver/linux.o
	#obj/ipxe/ipxe.o obj/driver/realtek.o \
	#obj/driver/linux.o obj/driver/r8169.o 
	#obj/driver/rtl8139.o obj/driver/realtek.o
	#obj/driver/r8168/r8168_n.o obj/driver/r8168/r8168_asf.o obj/driver/r8168/r8168_eeprom.o obj/driver/r8168/rtltool.o

LIBS = --start-group -ltlsf -ljsmn -lcore -llwip --end-group

all: kernel.elf

obj/%.o: src/%.asm
	mkdir -p $(DIR)
	nasm -f elf64 -o $@ $<

#obj/driver/r8169.o: src/driver/r8169.c
#	gcc -O2 -m64 -Wno-pointer-to-int-cast -ffreestanding -std=gnu99 -mcmodel=kernel -c -o $@ $<

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

kernel.elf: $(OBJS) $(LIBFILES)
	ld -melf_x86_64 -T elf_x86_64.ld -nostdlib -e main -o kernel.elf -L ../lib $^ $(LIBS)

clean:
	rm -rf obj
	rm -f kernel.bin
