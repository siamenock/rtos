.PHONY: all clean

all: rewrite

CFLAGS := -I ../../lib/core/include -Wall -Werror -std=gnu99

rewrite: src/rewrite.c
	gcc $(CFLAGS) -o $@ $<
	cp -f $@ ../../bin

clean:
	rm -f rewrite
