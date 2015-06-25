WARNINGS = -Wextra -Wwrite-strings -Wstrict-prototypes \
           -Wmissing-prototypes -Wno-long-long -Wstrict-aliasing=1

CFLAGS = -ffreestanding
CFLAGS += -g -O2 -I$(TLSF_PATH) $(WARNINGS) 
CFLAGS += -DUSE_MMAP=0
CFLAGS += -DUSE_SBRK=0
CFLAGS += -DUSE_PRINTF=0
CFLAGS += -DTLSF_STATISTIC=1
CFLAGS_32 := $(CFLAGS) -DTSLF_USE_LOCKS=0 -m32
CFLAGS += -DTLSF_USE_LOCKS=1 -m64 

CC=gcc

all: tlsf.o tlsf_32.o

tlsf.o: tlsf.h tlsf.c Makefile
	$(CC) $(CFLAGS) -c -o $@ tlsf.c 

tlsf_32.o: tlsf.h tlsf.c Makefile
	$(CC) $(CFLAGS_32) -c -o $@ tlsf.c 

clean:
	rm -rf *.o *~ *.c.gcov *.gcda *.gcno

indent:
	indent -kr -l120 -br -ce -npsl -nut -i4 *.[hc]

