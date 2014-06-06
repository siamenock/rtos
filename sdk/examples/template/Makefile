.PHONY: run all clean

CFLAGS = -I ../../include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = ../../lib/libpacketngin.a

app: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o app $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

all: app

clean:
	rm -rf obj
	rm -f $(APP)

run:
	../../bin/console script
