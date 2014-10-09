.PHONY: all clean mkver

CFLAGS = -I ../lib/TLSF/src -I ../lib/core/include -O0 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

DIR = obj obj/driver obj/rpc

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
	obj/rpc/rpc_manager.o obj/rpc/rpc_manager_xdr.o obj/rpc/rpc_callback.o obj/rpc/rpc_callback_xdr.o \
	obj/vm.o obj/manager.o

LIBS = --start-group -ltlsf -ljsmn -lcore --end-group -lxdr

LIBFILES = ../lib/libtlsf.a ../lib/libjsmn.a ../lib/libcore.a ../lib/libxdr.a

all: kernel.elf

mkver:
	./mkver.sh > src/version.h

$(DIR):
	mkdir -p $@

src/rpc/%.c: 
	make -C src/rpc

obj/rpc/%.o: src/rpc/%.c
	gcc -Wno-unused-variable $(CFLAGS) -c -o $@ $<

obj/shell.o: src/version.h

obj/apic.o: src/version.h

obj/%.o: src/%.asm
	nasm -f elf64 -o $@ $<

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

kernel.elf: mkver $(DIR) $(OBJS) $(LIBFILES)
	ld -melf_x86_64 -T elf_x86_64.ld -nostdlib -e main -o kernel.elf -L ../lib $(OBJS) $(LIBFILES) $(LIBS)

clean:
	make -C src/rpc clean
	rm -f src/version.h
	rm -rf obj
	rm -f kernel.bin
