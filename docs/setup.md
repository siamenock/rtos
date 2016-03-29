# Ubuntu 14.04 LTS (Desktop)
* Required Packages
  * git
  * nasm
  * multiboot
  * libcurl4-gnutls-dev
  * qemu-kvm
  * bridge-utils
  * libc6-dev-i386 
  * doxygen 
  * kpartx
  * bison
  * flex
  * cmake

* Disable automount-open
gsettings set org.gnome.desktop.media-handling automount-open false

# Setup bridge
Ref: http://toast.djw.org.uk/qemu.html

Edit file: /etc/network/interfaces
iface br0 inet manual
	bridge_ports eth0
	bridge_fd 9
	bridge_hello 2
	bridge_maxage 12
	bridge_stp off

# Install submodules

 - GNU Grub 
	cd rtos/tools/grub
	./autogen.sh
	./configure
	./make

 - Cmocka
	cd rtos/tools/cmocka
	mkdir ./build
	cd build && cmake ../ -DMAKE_INSTALL_PREFIX=/usr
	make & sudo make install


 
 


