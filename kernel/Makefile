.PHONY: all clean mkver

CFLAGS = -I ../lib/TLSF/src -I ../lib/core/include -I ../lib/lwip/include -I ../lib/lwip/src/include -I ../lib/lwip/src/include/ipv4 -O0 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -D_KERNEL_ #-DUSB_DEBUG

DIR = obj obj/driver obj/driver/usb

OBJS = obj/entry.o \
	obj/main.o obj/stdio.o obj/port.o obj/asm.o \
	obj/malloc.o obj/gmalloc.o obj/string.o obj/shared.o \
	obj/device.o obj/apic.o obj/apic_asm.o obj/cpu.o obj/mp.o obj/ioapic.o \
	obj/driver/vga.o obj/driver/keyboard.o

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
