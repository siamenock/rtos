.PHONY: all kernel clean doc

DIR = obj obj_linux obj_32

OBJ_DIR = obj/

OBJ_32_DIR = obj_32/

LINUX_DIR = obj_linux/

OBJS = ether.o crc.o checksum.o ni.o interface.o status.o  \
	timer.o fifo.o vector.o list.o map.o set.o ring.o errno.o thread.o \
	asm.o gmalloc.o lock.o shared.o json.o \
	malloc.o types.o _string.o _malloc.o rpc.o readline.o \
	arp.o icmp.o \
	packet.o ip.o udp.o tcp.o tftp.o cmd.o \
	fio.o file.o cache.o md5.o 

OBJS_32 := $(OBJS)	

OBJS += md5_asm.o event.o 

CFLAGS = -I include -I ../TLSF/src -I ../jsmn -O2 -Wall -Werror -nostdlib -ffreestanding -fno-stack-protector -std=gnu99

CFLAGS_32 := $(CFLAGS) -m32

CFLAGS += -mcmodel=kernel -m64 #-D__SSE_4_1__ -msse4.1

all: packetngin linux doc

packetngin: libcore.a libcore_32.a

linux: libcore_linux.a

obj/rpc.o: src/rpc.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -Wno-unused-variable -c -o $@ $<

obj_linux/rpc.o: src/rpc.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -Wno-unused-variable -DLINUX -c -o $@ $<

obj_32/rpc.o: src/rpc.c
	mkdir -p $(DIR)
	gcc $(CFLAGS_32) -Wno-unused-variable -DLINUX -c -o $@ $<

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

obj_32/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS_32) -c -o $@ $<

obj_32/%.o: src/%.s
	mkdir -p $(DIR)
	gcc $(CFLAGS_32) -c -o $@ $<

obj_32/%.o: src/%.asm
	mkdir -p $(DIR)
	nasm -f elf64 -o $@ $<

libcore_32.a: $(addprefix $(OBJ_32_DIR), $(OBJS_32))
	ar rcs $@ $^

doc:
	doxygen doxyfiles/Doxyfile

clean: 
	rm -rf doc
	rm -rf obj
	rm -f libcore.a
	rm -rf obj_32
	rm -f libcore_32.a
	rm -rf obj_linux
	rm -f libcore_linux.a
