#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "apic.h"
#include "page.h"

#define HANDLER_SIZE	256

static APIC_Handler handlers[HANDLER_SIZE];

uint64_t _apic_address;

int apic_init() {
	int fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(fd < 0) {
		perror("Failed to open memory descriptor\n");
		return -1;
	}

	printf("\tAssuming APIC physical base: %lx \n", _apic_address & ~(uint64_t)0xfffff);

	uint64_t _apic_address_page = (uint64_t)mmap((void*)(_apic_address & ~(uint64_t)0xfffff), PAGE_PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)(_apic_address & ~(uint64_t)0xfffff));

	if(_apic_address_page == (uint64_t)MAP_FAILED) {
		perror("\tMapping memory for absolute memory access failed.\n");
		return -1;
	}

	if(_apic_address_page != _apic_address) {
		perror("\tMapping memory is not same as APIC address.\n");
		return -1;
	}

	printf("\tMemory mapped APIC physcial base: %lx \n", _apic_address);

	return 0;
}

APIC_Handler apic_register(uint64_t vector, APIC_Handler handler) {
	APIC_Handler old = handlers[vector];
	handlers[vector] = handler;

	return old;
}

inline uint32_t apic_read32(int reg) {
	return *(uint32_t volatile*)(_apic_address + reg);
}

inline void apic_write32(int reg, uint32_t v) {
	*(uint32_t volatile*)(_apic_address + reg) = v;
}

inline uint64_t apic_read64(int reg) {
	uint64_t v = *(uint32_t volatile*)(_apic_address + reg);
	v |= (uint64_t)*(uint32_t volatile*)(_apic_address + reg + 0x10) << 32;

	return v;
}

inline void apic_write64(int reg, uint64_t v) {
	*(uint32_t volatile*)(_apic_address + reg + 0x10) = (uint32_t)(v >> 32);
	*(uint32_t volatile*)(_apic_address + reg) = (uint32_t)v;
}

