.PHONY: all kernel clean

DIR = obj obj/ds

OBJS = obj/ether.o obj/crc.o obj/checksum.o obj/ni.o obj/status.o obj/md5.o obj/md5_asm.o \
	obj/fifo.o obj/vector.o obj/list.o obj/map.o obj/ring.o obj/errno.o obj/thread.o \
	obj/asm.o obj/gmalloc.o obj/lock.o obj/shared.o obj/json.o obj/arp.o obj/icmp.o \
	obj/packet.o obj/ip.o obj/udp.o obj/tcp.o obj/rpc.o obj/tftp.o obj/event.o

CFLAGS = -I include -I ../TLSF/src -I ../jsmn -O0 -Wall -Werror -m64 -nostdlib -ffreestanding -std=gnu99 -mcmodel=kernel

all: libcore.a

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.S
	mkdir -p $(DIR)
	gcc -m64 -c -o $@ $<

obj/%.o: src/%.asm
	mkdir -p $(DIR)
	nasm -f elf64 -o $@ $<

libcore.a: $(OBJS)
	ar rcs $@ $^

clean: 
	rm -rf obj
	rm -f libcore.a
