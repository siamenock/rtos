.PHONY: all clean

all: pnkc/pnkc smap/smap #console/console 

pnkc/pnkc:
	make -C pnkc

smap/smap:
	make -C smap

umpn:
	make -C umpn

clean:
	make -C pnkc clean
	make -C smap clean
	make -C console clean
	make -C umpn clean
