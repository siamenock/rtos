.PHONY: all clean

CFLAGS = -I ../lib/core/include -I ../lib/lwip/src/include/ -I ../lib/lwip/src/include/ipv4 -I ../kernel/src/driver -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel

MODULES = r8169.ko rtl8139.ko

all: $(MODULES)

%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

%.ko: %.o
	mv $< $@

clean:
	rm -rf obj
	rm -f $(MODULES)
