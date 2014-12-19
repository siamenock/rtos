.PHONY: all clean mkver

CFLAGS = -I ../lib/TLSF/src -I ../lib/core/include -I ../lib/lwip/include -I ../lib/lwip/src/include -I ../lib/lwip/src/include/ipv4 -O0 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

DIR = obj obj/driver

OBJS = obj/entry.o \
	obj/main.o obj/stdio.o obj/shell.o obj/port.o obj/asm.o obj/gdt.o obj/idt.o \
	obj/isr.o obj/device.o obj/pci.o obj/malloc.o obj/gmalloc.o obj/task.o obj/task_asm.o \
	obj/mp.o obj/apic.o obj/apic_asm.o obj/ioapic.o obj/rtc.o obj/cpu.o \
	obj/acpi.o \
	obj/loader.o obj/shared.o obj/ni.o obj/icc.o \
	obj/string.o obj/module.o obj/symbols.o obj/rootfs.o \
	obj/driver/linux.o obj/driver/dummy.o \
	obj/driver/vga.o obj/driver/keyboard.o obj/driver/stdin.o obj/driver/stdout.o obj/driver/bfs.o \
	obj/driver/port.o \
	obj/vm.o obj/manager.o

LIBS = ../lib/libtlsf.a ../lib/libcore.a ../lib/liblwip.a

all: kernel.elf

src/version.h:
	./mkver.sh > src/version.h

$(DIR):
	mkdir -p $@

obj/shell.o: src/version.h

obj/apic.o: src/version.h

obj/%.o: src/%.asm
	nasm -f elf64 -o $@ $<

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

kernel.elf: src/version.h $(DIR) $(OBJS) $(LIBS)
	ld -melf_x86_64 -T elf_x86_64.ld -nostdlib -e main -o kernel.elf $(OBJS) $(LIBS)

clean:
	rm -f src/version.h
	rm -rf obj
	rm -f kernel.bin
