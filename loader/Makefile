.PHONY: all clean

all: loader.bin

obj/entry.bin: src/entry.asm
	mkdir -p obj
	nasm -o $@ $<

obj/%.o: src/%.asm
	mkdir -p obj
	nasm -f elf32 -o $@ $<

obj/%.o: src/%.c
	mkdir -p obj
	gcc -m32 -masm=intel -ffreestanding -std=gnu99 -c -o $@ $<

obj/loader.bin: obj/main.o obj/page.o obj/mode_switch.o
	ld -melf_i386 -T elf_i386.ld -nostdlib -e main -Ttext 0x10200 -o obj/loader.elf $^
	objcopy -j .text -j .data -j .rodata -j .bss -S -O binary obj/loader.elf $@

loader.bin: obj/entry.bin obj/loader.bin
	cat $^ > $@
	../util/truncate $@

clean:
	rm -rf obj
	rm -f loader.bin
