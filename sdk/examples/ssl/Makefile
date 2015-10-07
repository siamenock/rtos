.PHONY: run all clean
#-nostdinc 
CFLAGS = -I ../../../lib/newlib/newlib/libc/include -I ../../include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = --start-group ../../lib/libpacketngin.a ../../lib/libcrypto.a --end-group

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o main $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f main

run: all
	../../bin/console script
