.PHONY: all run deploy clean cleanall root.img

CLENLOG=rm -f /tmp/qemu.log
QEMU=qemu-system-x86_64 -m 256 -hda root.img -M pc -smp 8 -d cpu_reset -no-reboot -no-shutdown -monitor stdio -net nic,model=rtl8139 -net tap,script=util/qemu-ifup

all: root.img

boot/boot.bin:
	make -C boot

loader/loader.bin:
	make -C loader

kernel/kernel.elf:
	make -C kernel

util/rewrite:
	make -C util

util/pnkc:
	make -C util

util/smap:
	make -C util

LOADER_SIZE=$(shell stat -c%s loader/loader.bin)

# boot/boot.bin loader/loader.bin kernel/kernel.elf drivers/*.ko util/smap util/pnkc util/rewrite
root.img: 
	make -C boot
	make -C loader
	make -C kernel
	make -C drivers
	util/smap kernel/kernel.elf kernel.smap
	util/pnkc kernel.bin kernel/kernel.elf kernel.smap drivers/*.ko
	cat boot/boot.bin loader/loader.bin kernel.bin > $@
	util/truncate $@
	util/rewrite $@ $(LOADER_SIZE)

sdk: root.img lib
	rm -rf packetngin
	cp -r skel packetngin
	cp -r control/bin/* packetngin/bin/
	cp util/qemu-ifup packetngin/lib/
	cp root.img packetngin/lib/
	cp lib/libpacketngin.a packetngin/lib/
	cp -r lib/core/include packetngin
	tar cfz packetngin_sdk.tgz packetngin

run: root.img
	sudo $(QEMU) 

cli: root.img
	sudo $(QEMU) -curses

vnc: root.img
	sudo $(QEMU) -vnc :0

debug: root.img
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

deploy: root.img
	dd if=root.img of=/dev/sdb && sync

clean:
	rm -f root.img kernel.smap kernel.bin kernel.dis

cleanall: clean
	make -C boot clean
	make -C loader clean
	make -C kernel clean
	make -C lib clean
