.PHONY: all clean

all: boot.bin

boot.bin: src/boot.asm
	nasm -o $@ $<

clean:
	rm -f boot.bin
