.PHONY: all clean

CFLAGS = -I../lib/core/include -I../kernel/src/driver -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -I./linux

MODULES = virtio.ko fat.ko

all: $(MODULES) 

virtio.ko: linux.ko
	make -C virtio
	cp virtio/virtio.ko .

fat.ko: linux.ko
	make -C fat
	cp fat/fat.ko .

linux.ko: 
	make -C linux

clean:
	make -C linux clean
	make -C virtio clean
	make -C fat clean
	rm -rf obj
	rm -f *.ko
