.PHONY: all run deploy clean cleanall system.img mount umount

CLENLOG=rm -f /tmp/qemu.log
QEMU=qemu-system-x86_64 -enable-kvm -cpu host -m 256 -hda system.img -M pc -smp 8 -d cpu_reset -no-reboot -no-shutdown -monitor stdio -net nic,model=rtl8139 -net tap,script=util/qemu-ifup

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
	sudo umount mnt
	sudo losetup -d /dev/loop0
	rmdir mnt
	cat boot/boot.bin loader/loader.bin root.img > $@
	#util/truncate $@
	util/rewrite $@
	cp $@ sdk

mount:
	mkdir mnt
	sudo losetup /dev/loop0 root.img
	sudo mount /dev/loop0 mnt

umount:
	sudo umount mnt
	sudo losetup -d /dev/loop0
	rmdir mnt

sdk: system.img lib
	rm -rf packetngin
	cp -r skel packetngin
	cp -r control/bin/* packetngin/bin/
	cp util/qemu-ifup packetngin/lib/
	cp system.img packetngin/lib/
	cp lib/libpacketngin.a packetngin/lib/
	cp -r lib/core/include packetngin
	tar cfz packetngin_sdk.tgz packetngin

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
	dd if=system.img of=/dev/sdb && sync

clean:
	rm -f system.img root.img kernel.smap kernel.bin kernel.dis

cleanall: clean
	make -C boot clean
	make -C loader clean
	make -C kernel clean
	make -C drivers clean
	make -C lib cleanall
