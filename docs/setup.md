How to Build PacketNgin RTOS 
============================
This document describes required packages and steps to build PacketNgin RTOS.


Requirements 
------------
You need to install these packages below first. Theses packages are based on 
Ubuntu 14.04/16.04 LTS.

* git
* nasm
* multiboot
* libcurl4-gnutls-dev
* qemu-kvm
* bridge-utils
* libc6-dev-i386 
* doxygen 
* graphviz
* kpartx
* bison
* flex
* cmake
* node.js
* autoconf
* automake
* dcfldd
* libcmocka-dev

Build steps
-----------
0. We presume your RTOS root source tree is $RTOS.

1. Initilize git submodules. We have only one submodule 'GNU grub' to be tracked because it's very susceptible to each system. So you have to fetch it from source and install first.
	`cd $RTOS`
	`git submodule init`
	`git submodule update`

	`cd $RTOS/tools/grub`
	`./autogen.sh`
	`./configure`
	`make`

2. We use premake tool to generate Makefile. We provide premake binary to execute it.
	`cd $RTOS`
	`bin/premake5 gmake`

3. Install all required packages. We have helper Makefile to do it.
	`cd $RTOS`
	`make -f util.mk prepare`

4. Build PacketNgin RTOS system image.
	`cd $RTOS`
	`make`

5. After you build successfully, you can find 'system.img' in $RTOS.

6. Load RTOS system in QEMU
	`cd $RTOS`
	`make -f util.mk run`

Miscellaneous
-------------
Since we mount loop device to build system image whenever you build annoying nautilus window is appeared. Enter this command to disable this automount-open.

	`gsettings set org.gnome.desktop.media-handling automount-open false`
