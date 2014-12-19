.PHONY: all kernel clean

DIR = obj obj_linux

OBJ_DIR = obj/

LINUX_DIR = obj_linux/

OBJS = ether.o crc.o checksum.o ni.o status.o  md5.o md5_asm.o \
	fifo.o vector.o list.o map.o ring.o errno.o thread.o \
	asm.o gmalloc.o lock.o shared.o json.o arp.o icmp.o \
	packet.o ip.o udp.o tcp.o tftp.o event.o cmd.o \
	malloc.o types.o _string.o rpc.o

CFLAGS = -I include -I ../TLSF/src -I ../jsmn -O2 -Wall -Werror -m64 -nostdlib -ffreestanding -std=gnu99 -mcmodel=kernel

all: packetngin linux

packetngin: libcore.a

linux: libcore_linux.a

obj/rpc.o: src/rpc.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -Wno-unused-variable -c -o $@ $<

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.S
	mkdir -p $(DIR)
	gcc -m64 -c -o $@ $<

obj/%.o: src/%.asm
	mkdir -p $(DIR)
	nasm -f elf64 -o $@ $<

libcore.a: $(addprefix $(OBJ_DIR), $(OBJS))
	ar rcs $@ $^

obj_linux/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -DLINUX -c -o $@ $<

obj_linux/%.o: src/%.s
	mkdir -p $(DIR)
	gcc $(CFLAGS) -DLINUX -c -o $@ $<

obj_linux/%.o: src/%.asm
	mkdir -p $(DIR)
	nasm -f elf64 -o $@ $<

libcore_linux.a: $(addprefix $(LINUX_DIR), $(OBJS))
	ar rcs $@ $^

clean: 
	rm -rf obj
	rm -f libcore.a
	rm -rf obj_linux
	rm -f libcore_linux.a
