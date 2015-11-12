# PacketNgin Console

Sungho Kim (sungho@gurum.cc)
2015-09-11

---

##Getting Started
A step-by-step guide to getting up and running quickly.

	# cd ./os/tools/console
    # make
	# ./console
    > 
##Command Line Interface
The CLI documentation<br>
####exit
Exit the console
####quit
Exit the console
####help
Show this message
####echo [variable]
Print variable
####sleep [n]
Sleep n seconds
#### connect [[host] [port]]
Connect to the host
#### ping [count]
RPC ping to host
#### create [[vmid: uint32][core: uint8][memory: uint32][storage: uint32] [nic: [mac: uint64] [dev: string] [ibuf: uint32] [obuf: uint32] [iband: uint64] [oband: uint64] [pool: uint32] [args: string]]
Create Virtual Machine
####delete [vmid]
Delete VM
####list
List VM
####upload [[vmid] [path]]
Upload VM
####download [[vmid] [path]]
Download file
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
####stdin [vmid] [threadid] ["string"]
Send standard input Message


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