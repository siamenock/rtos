.PHONY: all kernel clean

DIR = obj

OBJ_DIR = obj/

OBJS = command.o vnic.o

#CFLAGS = -I include -g -O2 -Wall -Werror -m64 -nostdlib -ffreestanding -std=gnu99 -mcmodel=kernel
CFLAGS = -I include -g -O2 -Wall -Werror -m64 -std=gnu99

all: libvnic.a test

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

libvnic.a: $(addprefix $(OBJ_DIR), $(OBJS))
	ar rcs $@ $^

test: libvnic.a
	gcc -o $@ $<

clean: 
	rm -rf obj
	rm -f libvnic.a test

run: test
	./test
