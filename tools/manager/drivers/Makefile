# GNU Makefile for PacketNgin kernel disptacher
obj-m			:= dispatcher.o
ROOT_DIR		= $(PWD)/../../../

## Dispatcher
DISPATCHER_DIR		= dispatcher
dispatcher-objs		+= $(DISPATCHER_DIR)/dispatcher.o
dispatcher-objs		+= $(DISPATCHER_DIR)/nicdev.o
dispatcher-objs		+= ./../../../lib/libvnic.a

DISPATCHER_CFLAGS	= -std=gnu99 -Wno-strict-prototypes -Wno-declaration-after-statement -Wno-format
# TODO fix include dir
DISPATCHER_INCLUDE_DIR	= -I$(PWD)/dispatcher -I$(ROOT_DIR)/lib/vnic/include
CFLAGS_dispatcher.o	:= $(DISPATCHER_CFLAGS) $(DISPATCHER_INCLUDE_DIR)
CFLAGS_nicdev.o		:= $(DISPATCHER_CFLAGS) $(DISPATCHER_INCLUDE_DIR)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
