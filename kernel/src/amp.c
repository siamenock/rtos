#include "amp.h"

void cpuid(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
	asm volatile("cpuid"
		: "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
		: "a"(*a), "b"(*b), "c"(*c), "d"(*d));
}

uint8_t get_apic_id() {
 	uint32_t a, b, c, d;
	a = 0x01;
	cpuid(&a, &b, &c, &d);

	return (b >> 24) & 0xff;
}
