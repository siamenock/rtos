.PHONY: all clean

CFLAGS := -I ../../lib/core/include -Wall -Werror -std=gnu99

all: console

console: src/console.c
	gcc $(CFLAGS) -Wno-unused-variable -o $@ $^ -lcurl -DLINUX ../../lib/libumpn.a
	cp -f $@ ../../bin

clean:
	rm -f console
