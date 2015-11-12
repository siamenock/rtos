# PacketNgin Shell

Sungho Kim (sungho@gurum.cc)
2015-09-11

---
##Command Line Interface
The CLI documentation<br>
####help
Show this message
####version
Print the kernel version
####turbo [on/off]
Enable or Disable the turbo boost
####clear
Clear screen
####echo [variable]
Echo arguments
####sleep [n]
Sleep n secons
####date
Print current date and time
####manager [sub command]
Set ip, port, netmask, gateway, nic of manager
#####Sub commands
	ip [host]
    port [port]
	netmask [mask]
    gateway [gateway]
	nic [dev]
####nic
List, up, down, manager of network interface
####vnic
List of virtual network interface
####vlan
Add or remove vlan
####reboot
Reboot the PacketNgin
####shutdown
Shutdown the PacketNgin
####hatl
Halt the PacketNgin
####arping [host]
ARP ping to the host
#### create [[vmid: uint32][core: uint8][memory: uint32][storage: uint32] [nic: [mac: uint64] [dev: string] [ibuf: uint32] [obuf: uint32] [iband: uint64] [oband: uint64] [pool: uint32] [args: string]]
Create Virtual Machine
####delete [vmid]
Delete VM
####list
List VM
####send [[vmid] [path]]
Send file
####md5 [[vmid] [size]]
MD5 Storage
####start [vmid]
Start Virtual Machine
####pause [vmid]
Pause Virtual Machine
####resume [vmid]
Resume Virtual Machine
####stop [vmid]
Stop Virtual Machine
####status [vmid]
Print status of Virtual Machine

##Glossary
A quick lookup list of PacketNgin Network Emulator terminology.

	vm - Virtual Machine
    vmid - Virtual Machine ID Number
	md5 - md5 checksum
    nic - Network Interface Card
	dev - Network Interface Device name
	ibuf - input buffer
    obuf - output buffer
	iband - input max bandwidth
    oband - output max bandwidth
	pool - memory pool size

##FAQ
Frequently Asked Questions.

##Contact
Just send a inquiry to <contact@gurum.cc>
GurunmNetworks [http://gurum.cc](http://gurum.cc)
PacketNgin Opensource [http://packetngin.org](http://packetngin.org)


##Licenses
Learn about how we contribute to various free and open source software projects.