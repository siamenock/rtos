.PHONY: all clean

all: rewrite/rewrite pnkc/pnkc smap/smap console/console 

rewrite/rewrite:
	make -C rewrite

pnkc/pnkc:
	make -C pnkc

smap/smap:
	make -C smap

console/console:
	make -C console

umpn/packetngin:
	make -C umpn

clean:
	make -C rewrite clean
	make -C pnkc clean
	make -C smap clean
	make -C console clean
	make -C umpn clean
