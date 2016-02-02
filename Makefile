.PHONY: all run deploy clean cleanall system.img mount umount

# QEMU can select boot device - USB MSC, HDD
USB = -drive if=none,id=usbstick,file=./system.img -usb -device usb-ehci,id=ehci -device usb-storage,bus=ehci.0,drive=usbstick

HDD = -hda system.img

NIC = virtio #rtl8139

QEMU = qemu-system-x86_64 $(shell tools/qemu-params) -m 1024 -M pc -smp 8 -d cpu_reset -net nic,model=$(NIC) -net tap,script=tools/qemu-ifup -net nic,model=$(NIC) -net tap,script=tools/qemu-ifup $(USB) --no-shutdown --no-reboot  #$(HDD)

all: system.img

system.img: 
	make -C lib
	mkdir -p bin
	make -C tools
	make -C boot
	make -C loader
	make -C kernel
	make -C drivers
	# Make system map and kernel
	bin/smap kernel/kernel.elf kernel.smap
	bin/pnkc kernel/kernel.elf kernel.smap kernel.bin
	# Make init ran disk image
	tools/mkimage initrd.img 1 drivers/*.ko
	# Make system.img
	tools/mkimage system.img 64 3 12 fat32 fat32 ext2 loader/loader.bin kernel.bin initrd.img

mount:
	mkdir mnt
	sudo losetup /dev/loop0 root.img
	sudo mount /dev/loop0 mnt

umount:
	sudo umount mnt
	sudo losetup -d /dev/loop0
	rmdir mnt

ver:
	@echo $(shell git tag).$(shell git rev-list HEAD --count)

sdk: system.img
	cp $^ sdk/
	tar cfz packetngin_sdk-$(shell git tag).$(shell git rev-list HEAD --count).tgz sdk

virtualbox: system.img 
	$(eval UUID = $(shell VBoxManage showhdinfo system.vdi | grep UUID | awk '{print $$2}' | head -n1))
	rm -f system.vdi
	VBoxManage convertfromraw system.img system.vdi --format VDI --uuid $(UUID)
	VBoxManage startvm PacketNgin

run: system.img
	sudo $(QEMU) -monitor stdio

cli: system.img
	sudo $(QEMU) -curses

vnc: system.img
	sudo $(QEMU) -monitor stdio -vnc :0

debug: system.img
	sudo $(QEMU) -monitor stdio -S -s 

gdb:
	# target remote localhost:1234
	# set architecture i386:x86-64
	# file kernel/kernel.elf
	gdb --eval-command="target remote localhost:1234; set architecture i386:x86-64; file kernel/kernel.elf"

dis: kernel/kernel.elf
	objdump -d kernel/kernel.elf > kernel.dis && vi kernel.dis

stop:
	sudo killall -9 qemu-system-x86_64

deploy: system.img
	tools/deploy

clean:
	rm -f system.img root.img kernel.smap kernel.bin kernel.dis packetngin_sdk-*.tgz
	make -C kernel clean 
	make -C drivers clean

cleanall: clean
	rm -rf bin
	make -C boot clean
	make -C loader clean
	make -C kernel clean
	make -C drivers clean
	make -C lib cleanall
	make -C tools clean
