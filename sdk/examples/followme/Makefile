.PHONY: all clean

CFLAGS = -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = ../../sdk/libpn.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o followme $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f followme
