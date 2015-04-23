.PHONY: all clean

CFLAGS = -I../lib/core/include -I../kernel/src/driver -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -I./linux

MODULES = virtio.ko r8169.ko #rtl8139.ko #rtl8139.ko nf10.ko

all: $(MODULES) 

%.ko: %.o linux.ko
	ld -r $< ./linux/linux.ko -o $@

%.o: src/%.c 
	gcc $(CFLAGS) -c -o $@ $<

virtio.ko:
	make -C virtio
	cp virtio/virtio.ko .

nf10.ko: linux.ko
	make -C nf10
	cp nf10/nf10.ko .

linux.ko: 
	make -C linux

clean:
	make -C nf10 clean
#make -C linux clean
	make -C virtio clean
	rm -rf obj
	rm -f $(MODULES)
