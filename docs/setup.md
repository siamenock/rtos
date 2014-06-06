Ref: http://toast.djw.org.uk/qemu.html

apt-get install bridge-utils

/etc/network/interfaces
iface br0 inet manual
	bridge_ports eth0
	bridge_fd 9
	bridge_hello 2
	bridge_maxage 12
	bridge_stp off
