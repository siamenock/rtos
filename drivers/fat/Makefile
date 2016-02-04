.PHONY: all clean

CFLAGS = -I../linux -I../../kernel/src/driver -I../../lib/core/include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

OBJS =	obj/initfs.o \
	obj/common.o \
	obj/crtdef.o \
	obj/fatfs.o \
	obj/fat.o \
	obj/fat_file.o \
	obj/dir.o \
	obj/dirent.o \
	obj/hai_file.o \
	obj/cache.o \
	obj/debug.o \

obj/%.o: src/%.c 
	mkdir -p obj
	gcc $(CFLAGS) -c -o $@ $<

fat.ko: $(OBJS)
	ld -r ../linux/linux.ko $(OBJS) -o $@ 

all: fat.ko

clean:
	rm -rf obj
	rm -f fat.ko
