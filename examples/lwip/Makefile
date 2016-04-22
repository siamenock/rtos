.PHONY: run all clean

CFLAGS = -I ../../include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = -L ../../lib --start-group -lpacketngin -llwip --end-group
#../../lib/libpacketngin.a ../../lib/liblwip.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o lwip $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f lwip

run: all
	../../bin/console script
