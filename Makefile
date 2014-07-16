.PHONY: all run deploy clean cleanall system.img mount umount

QEMU=qemu-system-x86_64 $(shell util/qemu-params) -m 256 -hda system.img -M pc -smp 8 -d cpu_reset -no-shutdown -monitor stdio -net nic,model=rtl8139 -net tap,script=util/qemu-ifup

all: system.img

SYSTEM_IMG_SIZE := 1023		# 512 bytes * 1023 blocks = 512KB - 512B (for boot loader)

system.img: 
	make -C lib
	make -C util
	make -C boot
	make -C loader
	make -C kernel
	make -C drivers
	util/smap kernel/kernel.elf kernel.smap
	util/pnkc kernel/kernel.elf kernel.bin
	# Make root.img
	dd if=/dev/zero of=root.img count=$(SYSTEM_IMG_SIZE)
	mkdir mnt
	sudo losetup /dev/loop0 root.img
	sudo util/mkfs.bfs /dev/loop0
	sudo mount /dev/loop0 mnt
	sudo cp kernel.bin kernel.smap drivers/*.ko mnt
	sync
	sudo umount mnt
	sudo losetup -d /dev/loop0
	rmdir mnt
	cat boot/boot.bin loader/loader.bin root.img > $@
	util/rewrite $@

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

run: system.img
	sudo $(QEMU) 

cli: system.img
	sudo $(QEMU) -curses

vnc: system.img
	sudo $(QEMU) -vnc :0

debug: system.img
	sudo $(QEMU) -S -s 

gdb:
	# target remote localhost:1234
	# set architecture i386:x86-64
	# file kernel/kernel.elf
	gdb --eval-command="target remote localhost:1234; set architecture i386:x86-64; file kernel/kernel.elf"

dis: kernel/kernel.elf
	objdump -d kernel/kernel.elf > kernel.dis && vi kernel.dis

stop:
	killall -9 qemu-system-x86_64

deploy: system.img
	util/chk-sdb
	sudo dd if=system.img of=/dev/sdb && sync

clean:
	rm -f system.img root.img kernel.smap kernel.bin kernel.dis packetngin_sdk-*.tgz

cleanall: clean
	make -C boot clean
	make -C loader clean
	make -C kernel clean
	make -C drivers clean
	make -C lib cleanall
