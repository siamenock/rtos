.PHONY: all clean

all: pnkc

CFLAGS := -I ../../lib/core/include -Wall -Werror -std=gnu99

pnkc: src/pnkc.c
	gcc $(CFLAGS) -o $@ $<
	cp -f $@ ../../bin

clean:
	rm -f pnkc
