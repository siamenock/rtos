# Ubuntu 14.04 LTS (Desktop)
* Required Packages
  * git
  * nasm
  * libcurl4-gnutls-dev
  * qemu-kvm
  * bridge-utils
  * libc6-dev-i386 
  * doxygen 

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
