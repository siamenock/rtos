.PHONY: all clean

all: pnkc/pnkc smap/smap #console/console 

pnkc/pnkc:
	make -C pnkc

smap/smap:
	make -C smap

#console/console:
	#make -C console

#TODO : UMPN should be moved
umpn/packetngin:
	make -C umpn

clean:
	make -C pnkc clean
	make -C smap clean
	make -C console clean
	make -C umpn clean
