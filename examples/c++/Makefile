.PHONY: run all clean

CFLAGS = -I ../../include -I ../../../zlib -O2 -Wall -Werror -m64 -ffreestanding

DIR = obj

OBJS = obj/main.o

LIBS = ../../lib/libpacketngin.a ../../lib/zlib/libz.a

app: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o app $^ $(LIBS)

obj/%.o: src/%.cpp
	mkdir -p $(DIR)
	g++ $(CFLAGS) -c -o $@ $<

all: app

clean:
	rm -rf obj
	rm -f $(APP)

run:
	../../bin/console script
