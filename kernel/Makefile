.PHONY: all clean mkver

CFLAGS = -I ../lib/TLSF/src -I ../lib/core/include -I ../lib/lwip/include -I ../lib/lwip/src/include -I ../lib/lwip/src/include/ipv4 -O0 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -D_KERNEL_ #-DUSB_DEBUG

DIR = obj obj/driver obj/driver/usb

OBJS = obj/entry.o \
	obj/main.o obj/stdio.o obj/port.o obj/asm.o \
	obj/malloc.o obj/gmalloc.o obj/string.o obj/shared.o \
	obj/device.o obj/apic.o obj/apic_asm.o obj/cpu.o obj/mp.o obj/ioapic.o \
	obj/gdt.o obj/idt.o obj/isr.o obj/acpi.o obj/pci.o obj/task.o obj/task_asm.o \
	obj/symbols.o obj/icc.o \
	\
	obj/driver/vga.o obj/driver/keyboard.o \
	obj/driver/disk.o obj/driver/pata.o obj/driver/bfs.o obj/driver/ramdisk.o \
	obj/driver/fs.o obj/driver/file.o \
	obj/driver/port.o \
	\
	obj/driver/usb/usbinit.o obj/driver/usb/usb.o obj/driver/usb/usb_dev.o \
	obj/driver/usb/generic_hub.o obj/driver/usb/usbhub.o \
	obj/driver/usb/ehci.o obj/driver/usb/ehci_rh.o \
	obj/driver/usb/pci.o obj/driver/usb/quirks.o \
	obj/driver/usb/__udivdi3.o  obj/driver/usb/__umoddi3.o \
	obj/driver/usb/usbhid.o obj/driver/usb/usbmsc.o \
	obj/driver/usb/usbclient.o \
	obj/driver/usb/video.o obj/driver/usb/console.o obj/driver/usb/vga.o \
	obj/driver/usb/ctype.o

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
