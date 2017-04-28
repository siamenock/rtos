.PHONY: all test clean cleanall

CC = gcc
CFLAGS = -O2

all: libvnic.a

CC=gcc
CFLAGS=-I include -O2 -Wall -mcmodel=large -fno-stack-protector -fno-common

SRCS=lock.c vnic.c nic.c asm.asm
OBJS=$(addsuffix .o, $(addprefix obj/, $(basename $(SRCS))))
TESTS=$(addsuffix _test.o, $(addprefix obj/, $(basename $(SRCS))))

libvnic.a: $(OBJS)
	ar rcv $@ $^
	cp $@ ../

obj/asm.o: src/asm.asm
	-mkdir -p obj
	nasm -f elf64 -o $@ $^

obj/%.o: src/%.c
	-mkdir -p obj
	$(CC) $(CFLAGS) $^ -c -o $@

obj/asm_test.o: src/asm.asm
	-mkdir -p obj
	nasm -f elf64 -o $@ $^

obj/%_test.o: src/%.c
	-mkdir -p obj
	$(CC) -g -DTEST $(CFLAGS) $^ -c -o $@

test: $(TESTS)
	$(CC) $^ -o test
	./$@

clean: 
	rm -rf test
	rm -rf obj
	rm -rf libvnic.a

cleanall: clean
