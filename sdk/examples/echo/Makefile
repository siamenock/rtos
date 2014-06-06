.PHONY: run all clean

CFLAGS = -I ../../include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99

DIR = obj

OBJS = obj/main.o

LIBS = ../../lib/libpacketngin.a

all: $(OBJS)
	ld -melf_x86_64 -nostdlib -e main -o main $^ $(LIBS)

obj/%.o: src/%.c
	mkdir -p $(DIR)
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -rf obj
	rm -f main

run: all
	curl -v -d '{ core_num: 1, memory_size: 0x1000000, storage_size: 0x200000, nics: [ { mac: "00:11:22:33:44:55", input_buffer_size: 1024, output_buffer_size: 1024, input_bandwidth: 1000000000, output_bandwidth: 1000000000, pool_size: 0x400000 } ], args: [ "hello", "world" ] }' http://192.168.100.254/vm
	curl -v -X POST --data-binary @main http://192.168.100.254/vm/1/storage
	curl -v -X PUT -d 'start' http://192.168.100.254/vm/1/power

deploy: all
	curl -v -d '{ core_num: 1, memory_size: 0x1000000, storage_size: 0x200000, nics: [ { mac: "00:11:22:33:44:55", input_buffer_size: 1024, output_buffer_size: 1024, input_bandwidth: 1000000000, output_bandwidth: 1000000000, pool_size: 0x400000 } ], args: [ "hello", "world" ] }' http://192.168.200.254/vm
	curl -v -X POST --data-binary @main http://192.168.200.254/vm/1/storage
	curl -v -X PUT -d 'start' http://192.168.200.254/vm/1/power
