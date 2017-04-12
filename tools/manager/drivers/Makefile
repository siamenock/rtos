# GNU Makefile for PacketNgin kernel disptacher
obj-m			:= dispatcher.o

## Dispatcher
DISPATCHER_DIR		= dispatcher
dispatcher-objs		+= $(DISPATCHER_DIR)/dispatcher.o
dispatcher-objs		+= $(DISPATCHER_DIR)/vnic.o
dispatcher-objs		+= ../penguin/lib/libumpn.a

DISPATCHER_CFLAGS	= -std=gnu99 -Wno-strict-prototypes -Wno-declaration-after-statement -Wno-format
CFLAGS_vnic.o		:= $(DISPATCHER_CFLAGS)
CFLAGS_dispatcher.o	:= $(DISPATCHER_CFLAGS)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
