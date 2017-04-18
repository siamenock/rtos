# GNU Makefile for PacketNgin kernel disptacher  
obj-m			:= virtio_accel.o

## VirtI/O specific
VIRTIO_DIR		= virtio
virtio_accel-objs	+= $(VIRTIO_DIR)/virtio_net.o

## Dispatcher specificcific
DISPATCHER_DIR		= dispatcher
virtio_accel-objs	+= $(DISPATCHER_DIR)/dispatcher.o 
virtio_accel-objs	+= $(DISPATCHER_DIR)/vnic.o
virtio_accel-objs	+= ../libumpn.a

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
