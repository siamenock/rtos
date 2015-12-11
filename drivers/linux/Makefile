.PHONY: all clean 

CFLAGS = -I. -I../../kernel/src -I../../lib/core/include -O2 -Wall -Werror -m64 -ffreestanding -std=gnu99 -mcmodel=kernel #-nostdinc

OBJS = obj/cdev.o 		\
	obj/atomic.o  		\
	obj/delay.o		\
	obj/device.o		\
	obj/slab.o		\
	obj/dma-mapping.o	\
	obj/etherdevice.o	\
	obj/fs.o 		\
	obj/interrupt.o 	\
	obj/io.o 		\
	obj/ioport.o 		\
	obj/jiffies.o		\
	obj/netdevice.o		\
	obj/skbuff.o		\
	obj/spinlock.o		\
	obj/string.o		\
	obj/time.o		\
	obj/workqueue.o		\
	obj/pci.o		\
	obj/uaccess.o    	\
	obj/err.o		\
	obj/bitops.o		\
	obj/mutex.o		\
	obj/pci_dma_compat.o	\
	obj/net.o		\
	obj/firmware.o 		\
	obj/timer.o		\
	obj/printk.o		\
	obj/time64.o

obj/%.o: src/%.c 
	mkdir -p obj
	gcc $(CFLAGS) -c -o $@ $<

linux.ko: $(OBJS)
	ld -r $(OBJS) -o $@

all: linux.ko

clean:
	rm -rf obj
	rm -f linux.ko
	
