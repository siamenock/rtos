.PHONY: all clean

CFLAGS = -I../lib/TLSF/src -I../lib/core/include -m32 -masm=intel -ffreestanding -std=gnu99 #-DUSB_DEBUG

DIR = obj obj/driver obj/driver/usb 

USB = obj/driver/usb/usbinit.o obj/driver/usb/usb.o obj/driver/usb/usb_dev.o \
      obj/driver/usb/generic_hub.o obj/driver/usb/usbhub.o \
      obj/driver/usb/ehci.o obj/driver/usb/ehci_rh.o \
      obj/driver/usb/pci.o obj/driver/usb/quirks.o \
      obj/driver/usb/__udivdi3.o  obj/driver/usb/__umoddi3.o \
      obj/driver/usb/usbhid.o obj/driver/usb/usbmsc.o \
      obj/driver/usb/usbclient.o \
      obj/driver/usb/video.o obj/driver/usb/console.o obj/driver/usb/vga.o \
      obj/driver/usb/ctype.o obj/driver/usb/printf.o \

OBJS = obj/main.o obj/page.o obj/mode_switch.o obj/driver/bfs.o obj/driver/pata.o \
       obj/port.o obj/driver/fs.o obj/string.o obj/cpu.o \
       obj/driver/disk.o obj/driver/file.o $(USB) 

LIBS = ../lib/libtlsf_32.a ../lib/libcore_32.a

all: loader.bin

$(DIR):
	mkdir -p $@

obj/entry.bin: src/entry.asm $(DIR)
	nasm -o $@ $<

obj/%.o: src/%.asm
	nasm -f elf32 -o $@ $<

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/loader.bin: $(OBJS) $(LIBS)
	ld -melf_i386 -T elf_i386.ld -nostdlib -e main -Ttext 0x10200 -o obj/loader.elf $^ 
	objcopy -j .text -j .data -j .rodata -j .bss -S -O binary obj/loader.elf $@

loader.bin: obj/entry.bin obj/loader.bin
	cat $^ > $@
	../tools/truncate $@

clean:
	rm -rf obj
	rm -f loader.bin
