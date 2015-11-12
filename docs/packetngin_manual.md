# PacketNgin RTOS

Sanggyu Lee (sanggyu@gurum.cc)
2015-09-08

---


##Getting Started
PacketNgin Operating System is a DPI(Deep Packet Inspection) and network virtualization platform based on x86_64 architecture. 

An overview of PacketNgin RTOS, how to install and implement.

###PacketNgin RTOS Concept
Network O/S PacketNgin RTOS looks like a picture below. PacketNgin creates VM called real-time virtual machine in the form of container per every application. Virtual machine consists of several CPU cores, memory, and virtual network interface controller.

![](image/packetngin_concept_02.png)

###Why PacketNgin RTOS?
Compared to general purpose O/S like Linux which offers only payload to user, Network O/S like PacketNgin gives every header information to user. It gives easier and more programmable experience to user. So user application like firewall can manipulate the header information directly.

![](image/packetngin_concept_01.png)

###What's included
This is the most basic form of PacketNgin SDK. We provide examples directory which contains some Net App Examples. Also, we provide include and lib which are needed when Net App be complied. And PacketNgin image file is system.img.

> PacketNgin SDK
> bin/
>>console
>>deploy
>>qemu
> 
> examples/
>>arping
>>dpi
>>...
>
>include/
>>control
>>openssl
>>util
>>...
>
>lib/
>>libcrypto.a
>>libpacketngin.a
>>...
>
>system.img

##How to implement PacketNgin RTOS
Before you install PacketNgin, Linux OS is required such as Ubuntu, Fedora, Redhat, etc.. In this tutorial, we only consider Ubuntu 14.04 which we prefer than others.

###1. Install Required Package

	$ sudo apt-get install vim
	$ sudo apt-get install uml-utilities
	$ sudo apt-get install virtualbox

###2. Make and Set 'tap0' up

	$ sudo tunctl
    $ sudo vim/etc/network/interfaces

![](image/set_net.png)

###3. Restart network

	$ sudo ifup tap0

###4. Install PacketNgin SDK

	$ tar xvfz sdk.tar.gz

###5. VirtualBox Settings

	Press 'New' button
    
![](image/vbox_main.png)

	Name, set type and version as below
    
![](image/vbox_set_name.png)

	Set memory size. We recommend at least more than 1024 MB.
    
![](image/vbox_set_mem.png)

	Select 'Use an existing virtual hard disk file' then find system.vdi. It's in the SDK.
    
![](image/vbox_set_hdd.png)

	Press 'Settings' button
    
![](image/vbox_main_pkng.png)

	PacketNgin needs at least 2 cores and supports up to 8.
    
![](image/vbox_set_core.png)

	Set network settings as below
    
![](image/vbox_set_net.png)

	Success!
    
![](image/pkng_main.png)


##Contact
Just send a inquiry to <contact@gurum.cc>
GurunmNetworks [http://gurum.cc](http://gurum.cc)
PacketNgin Opensource [http://packetngin.org](http://packetngin.org)


##Licenses
PacketNgin O/S is licensed under both General Public License(GPL) version 2 and a proprietary license that can be arranged with us. Each component of PacketNgin O/S is licensed under different licesnes.

###The reason of dual-license and the role of GurumNetworks
PacketNgin is very carefully designed to use dual-licensing strategy. Most of the users are not necessary to use proprietary license(GPL license is very attractive to most of users). But some company need to make their own network device with their intellectual property need alternative license rather than GPL.

The role of GurumNetworks, Inc. is make a virtuous cycle to develop the product intensively and continuously. We promise most of income from commercial licensing will be reinvested to develop OSS version of the product.

###The components licensed under GPL2 and proprietary
PacketNgin Bootloader
PacketNgin Kernel
PacketNgin SDK except 3rd parth libraries

###The components licensed under GPL2 licensed with special exceptions and proprietary

PacketNgin SDK's core library

###The components licensed under 3rd party licenses

PacketNgin SDK's libraries except core library

###The license of loadable kernel modules

Loadable kernel module is not affected by GPL and it's the tradition of Linux kernel. PacketNgin Kernel follow the tradition too. PacketNgin's loadable kernel module doesn't affected by the PacketNgin kernel's license.

Please check the URL: http://linuxmafia.com/faq/Kernel/proprietary-kernel-modules.html