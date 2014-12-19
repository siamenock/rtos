.PHONY: all clean cleanall core newlib

all: libtlsf.a libjsmn.a libcore.a libcore_linux.a libc.a libm.a libz.a libssl.a liblwip.a libxdr.a libpacketngin.a

libtlsf.a: 
	make -C TLSF/src
	ar rcs $@ TLSF/src/tlsf.o

jsmn/libjsmn.a:
	make -C jsmn

libjsmn.a: jsmn/libjsmn.a
	cp jsmn/libjsmn.a .

core/libcore.a:
	make -C core

core/libcore_linux.a:
	make -C core

libcore.a: libjsmn.a libtlsf.a core/libcore.a
	cp core/libcore.a .

libcore_linux.a: core/libcore_linux.a
	cp core/libcore_linux.a .

core: libjsmn.a libtlsf.a
	make -C core
	cp core/libcore.a .
	cp core/libcore_linux.a .

newlib/Makefile: 
	cd newlib; CFLAGS=-fno-stack-protector ./configure --target=x86_64-pc-packetngin --disable-multilib

PACKETNGIN_BASE = $(shell pwd)

newlib/x86_64-pc-packetngin/newlib/libc.a: newlib/Makefile
	PATH=$$PATH:$(PACKETNGIN_BASE)/bin make -C newlib

libc.a: newlib/x86_64-pc-packetngin/newlib/libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libc.a .

libm.a: libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libm.a .

newlib: newlib/Makefile
	PATH=$$PATH:$(PACKETNGIN_BASE)/bin make -C newlib
	cp newlib/x86_64-pc-packetngin/newlib/libc.a .

libpacketngin.a: libc.a libm.a libtlsf.a libcore.a
	mkdir -p obj
	cd obj; ar x ../libc.a
	cd obj; ar x ../libm.a
	cd obj; ar x ../libtlsf.a
	cd obj; ar x ../libcore.a
	cd obj; ar rcs ../$@ *.o
	rm -r obj
	mkdir -p ../sdk/include
	cp -rL core/include/* ../sdk/include/
	cp libpacketngin.a ../sdk/lib/

libz.a: zlib/libz.a
	cd zlib; ./configure --static
	make -C zlib
	cp zlib/libz.a .

openssl/libssl.a: 
	cd openssl; ./Configure packetngin-x86_64 no-threads no-shared no-dso
	PACKETNGIN_BASE=$(PACKETNGIN_BASE) make -C openssl build_libs

libssl.a: openssl/libssl.a
	cp openssl/libssl.a .
	cp openssl/libcrypto.a .
	cp -rL openssl/include/* ../sdk/include/
	cp libssl.a ../sdk/lib/
	cp libcrypto.a ../sdk/lib/

liblwip.a: lwip/liblwip.a
	make -C lwip
	cp lwip/liblwip.a .
	
xdr/linux/libxdr.a:
	make -C xdr -f Makefile.unix linux/libxdr.a

libxdr.a: xdr/linux/libxdr.a
	cp xdr/linux/libxdr.a .

clean: 
	rm -f libtlsf.a libjsmn.a libcore.a libcore_linux.a libc.a libm.a libz.a libssl.a libcrypto.a liblwip.a libpacketngin.a

cleanall: clean
	-make -C TLSF/src clean
	-make -C jsmn clean
	-make -C core clean
	-make -C newlib distclean
	-make -C zlib distclean
	-make -C lwip clean
	-make -C openssl clean
	-make -C xdr -f Makefile.unix realclean
