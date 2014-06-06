.PHONY: all clean cleanall

all: libtlsf.a libjsmn.a libcore.a libc.a libm.a libz.a libssl.a liblwip.a libpacketngin.a

libtlsf.a: TLSF/src/tlsf.o 
	make -C TLSF/src
	ar rcs $@ $^

jsmn/libjsmn.a:
	make -C jsmn

libjsmn.a: jsmn/libjsmn.a
	cp jsmn/libjsmn.a .

core/libcore.a:
	make -C core
	
libcore.a: libjsmn.a libtlsf.a core/libcore.a
	cp core/libcore.a .

newlib/Makefile: 
	cd newlib; ./configure --target=x86_64-pc-packetngin --disable-multilib

newlib/x86_64-pc-packetngin/newlib/libc.a: newlib/Makefile
	PATH=$${PATH}:../bin make -C newlib -j 16

libc.a: newlib/x86_64-pc-packetngin/newlib/libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libc.a .

libm.a: libc.a
	cp newlib/x86_64-pc-packetngin/newlib/libm.a .

libpacketngin.a: libc.a libm.a libtlsf.a libcore.a
	mkdir -p obj
	cd obj; ar x ../libc.a
	cd obj; ar x ../libm.a
	cd obj; ar x ../libtlsf.a
	cd obj; ar x ../libcore.a
	cd obj; ar rcs ../$@ *.o
	rm -r obj
	cp libpacketngin.a ../sdk/lib/

libz.a: zlib/libz.a
	cd zlib; ./configure
	make -C zlib
	cp zlib/libz.a .

PACKETNGIN_BASE = $(shell pwd)

openssl/Makefile:
	cd openssl; ./Configure packetngin-x86_64 no-threads no-shared no-dso

openssl/libssl.a: openssl/Makefile
	PACKETNGIN_BASE=$(PACKETNGIN_BASE)/.. make -C openssl build_libs

libssl.a: openssl/libssl.a
	cp openssl/libssl.a .
	cp openssl/libcrypto.a .
	cp -rL openssl/include/* ../sdk/include/
	cp libssl.a ../sdk/lib/
	cp libcrypto.a ../sdk/lib/

liblwip.a: lwip/liblwip.a
	make -C lwip
	cp lwip/liblwip.a .
	
clean: 
	rm -f libpacketngin.a
	rm -f libkernel.a
	rm -f libtlsf.a
	rm -f libcore.a
	rm -f libc.a
	rm -f libm.a

cleanall: clean
	rm -r newlib/x86_64-pc-packetngin/
	make -C TLSF/src clean
	make -C core clean
	#make -C newlib distclean
