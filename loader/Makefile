.PHONY: all clean

CFLAGS = -m32 -Wall -Werror -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/entry.o obj/main.o obj/print.o obj/time.o obj/apic.o

all: loader.bin

obj/entry_tmp.o: obj/entry_tmp.asm
	nasm -f elf32 -o $@ $<

obj/entry_tmp.asm: src/entry.asm
	mkdir -p $(DIR)
	./set_tmp

obj/entry.o: obj/entry.asm
	nasm -f elf32 -o $@ $<

obj/entry.asm: loader.tmp
	./set_value

obj/%.o: src/%.asm
	nasm -f elf32 -o $@ $<

obj/%.o: src/%.c
	gcc $(CFLAGS) -c -o $@ $<

obj/x64.o: src/x64.asm
	nasm -o $@ $<

kernel: ../kernel.bin
	cp $< $@

boot: src/boot.asm
	nasm -o $@ $<

loader.tmp: obj/entry_tmp.o $(filter-out obj/entry.o, $(OBJS))
	ld -melf_i386 -T linker.ld -e start -o $@ $^ 

loader.bin: loader.tmp $(OBJS)
	ld -melf_i386 -T linker.ld -e start -o $@ $(filter-out loader.tmp, $^)

clean:
	rm -rf obj
	rm -f loader.tmp
	rm -f loader.bin
