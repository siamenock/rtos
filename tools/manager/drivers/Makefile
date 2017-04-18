# GNU Makefile for PacketNgin kernel disptacher
obj-m			:= dispatcher.o
ROOT_DIR		= $(PWD)/../../../

## Dispatcher
DISPATCHER_DIR		= dispatcher
dispatcher-objs		+= $(DISPATCHER_DIR)/dispatcher.o
dispatcher-objs		+= $(DISPATCHER_DIR)/vvnic.o
# dispatcher-objs		+= $(DISPATCHER_DIR)/vnic.o
dispatcher-objs		+= ./../../../lib/libumpn.a  ./../../../lib/libnic.a

DISPATCHER_CFLAGS	= -std=gnu99 -Wno-strict-prototypes -Wno-declaration-after-statement -Wno-format
# TODO fix include dir
DISPATCHER_INCLUDE_DIR	= -I$(PWD)/dispatcher -I$(ROOT_DIR)/include -I$(ROOT_DIR)/lib/nic/include -I$(ROOT_DIR)/lib/core/include
DISPATCHER_LIB_DIR	= $(ROOT_DIR)/lib
 CFLAGS_vvnic.o		:= $(DISPATCHER_CFLAGS) $(DISPATCHER_INCLUDE_DIR) 
CFLAGS_dispatcher.o	:= $(DISPATCHER_CFLAGS) $(DISPATCHER_INCLUDE_DIR)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
