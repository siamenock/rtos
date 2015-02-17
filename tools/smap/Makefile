.PHONY: all clean

all: smap

CFLAGS := -I ../../lib/core/include -Wall -Werror -std=gnu99

smap: src/smap.c
	gcc $(CFLAGS) -o $@ $<
	cp -f $@ ../../bin

clean:
	rm -f smap
