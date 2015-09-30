.PHONY: all clean

CFLAGS = -I../lib/core/include -I../kernel/src/driver -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -I./linux

MODULES = virtio.ko

all: $(MODULES) 

virtio.ko: linux.ko
	make -C virtio
	cp virtio/virtio.ko .

linux.ko: 
	make -C linux

clean:
	make -C linux clean
	make -C virtio clean
	rm -rf obj
	rm -f *.ko
