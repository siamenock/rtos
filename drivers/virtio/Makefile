.PHONY: all clean

CFLAGS = -I. -I../../kernel/src/driver -I../../lib/core/include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

OBJS = obj/virtio.o

obj/%.o: %.c 
	mkdir -p obj
	gcc $(CFLAGS) -c -o $@ $<

virtio.ko: $(OBJS)
	ld -r ../linux/linux.ko $(OBJS) -o $@ 

all: virtio.ko

clean:
	rm -rf obj
	rm -f virtio.ko
