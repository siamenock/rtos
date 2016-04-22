.PHONY: all clean

CFLAGS = -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -I ../../include

DIR = obj

OBJS = obj/main.o

//LIBS = ../../sdk/libpn.a
LIBS = ../../lib/libpacketngin.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o followme $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f followme

run: all
	../../bin/console script
