.PHONY: all clean

CFLAGS = -I../lib/core/include -I../kernel/src/driver -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel -I./linux

MODULES = r8169.ko rtl8139.ko nf10.ko 

all: $(MODULES) 

%.ko: %.o linux.ko
	ld -r $< ./linux/linux.ko -o $@

%.o: src/%.c 
	gcc $(CFLAGS) -c -o $@ $<

nf10.ko: 
	make -C nf10
	cp nf10/nf10.ko .

linux.ko:
	make -C linux

clean:
	make -C nf10 clean
	make -C linux clean
	rm -rf obj
	rm -f $(MODULES)
