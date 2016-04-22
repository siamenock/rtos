.PHONY: all clean

CFLAGS = -I ../../include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = ../../lib/libpacketngin.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o iperf_server $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f iperf_server
