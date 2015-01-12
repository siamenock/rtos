.PHONY: all clean

all: rewrite pnkc smap console

CFLAGS := -I ../lib/core/include -Wall -Werror -std=gnu99

rewrite: src/rewrite.c
	gcc $(CFLAGS) -o $@ $<

pnkc: src/pnkc.c
	gcc $(CFLAGS) -o $@ $<

smap: src/smap.c
	gcc $(CFLAGS) -o $@ $<

console: src/console.c
	gcc $(CFLAGS) -Wno-unused-variable -o $@ $^ ../lib/libcore_linux.a -lcurl -DLINUX
	cp console ../sdk/bin/

clean:
	rm -f rewrite pnkc smap  console
