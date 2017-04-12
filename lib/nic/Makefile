.PHONY: all test clean cleanall

CC = gcc
CFLAGS = -O2

all: libnic.a

CC=gcc
CFLAGS=-I include -O2 -Wall

SRCS=lock.c nic.c asm.asm
OBJS=$(addsuffix .o, $(addprefix obj/, $(basename $(SRCS))))
TESTS=$(addsuffix _test.o, $(addprefix obj/, $(basename $(SRCS))))

libnic.a: $(OBJS)
	ar rcv $@ $^

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
	rm -rf libnic.a

cleanall: clean
