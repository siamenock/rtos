.PHONY: all clean

all: rewrite pnkc smap console

rewrite: src/rewrite.c
	gcc -g -std=gnu99 -o $@ $<

pnkc: src/pnkc.c
	gcc -g -std=gnu99 -o $@ $<

smap: src/smap.c
	gcc -g -std=gnu99 -o $@ $<

console: src/console.c
	gcc -g -std=gnu99 -o $@ $<

clean:
	rm -f rewrite pnkc
