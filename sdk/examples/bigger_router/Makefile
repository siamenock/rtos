.PHONY: all clean

CFLAGS = -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -I ../../lib/newlib/newlib/libc/include:../../lib/TLSF/src

DIR = obj

OBJS = obj/main.o

LIBS = ../../lib/newlib/x86_64-pc-packetngin/newlib/libc.a ../../sdk/libpn.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o bigger_router $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f bigger_router
