.PHONY: all lib boot loader kernel run gui stop deploy clean cleanall

CLENLOG=rm -f /tmp/qemu.log
QEMU=qemu-system-x86_64 -m 256 -hda root.img -M pc -smp 8 -d cpu_reset -no-reboot -no-shutdown -monitor stdio -net nic,model=rtl8139 -net tap,script=util/qemu-ifup

all: root.img

boot:
	make -C boot

loader:
	make -C loader

kernel: 
	make -C kernel

lib:
	make -C lib

util/rewrite:
	make -C util

LOADER_SIZE=$(shell stat -c%s loader/loader.bin)

root.img: lib boot loader kernel util/rewrite
	cat boot/boot.bin loader/loader.bin kernel/kernel.bin > $@
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
	$(CLEANLOG)
	sudo $(QEMU) 

cli: root.img
	$(CLEANLOG)
	sudo $(QEMU) -curses

vnc: root.img
	$(CLEANLOG)
	sudo $(QEMU) -vnc :0

guidebug: root.img
	$(CLEANLOG)
	sudo $(QEMU) -S -s 

gdb:
	# target remote localhost:1234
	# set architecture i386:x86-64
	# file kernel/obj/kernel.elf
	gdb --eval-command="target remote localhost:1234"

stop:
	killall -9 qemu-system-x86_64

deploy: root.img
	dd if=root.img of=/dev/sdb && sync

clean:
	rm -rf packetngin_sdk.tgz
	rm -rf packetngin
	rm -f root.img

cleanall: clean
	make -C boot clean
	make -C loader clean
	make -C kernel clean
	make -C lib clean
