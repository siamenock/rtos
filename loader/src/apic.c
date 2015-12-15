#include "apic.h"

static uint32_t apic_address;

void apic_init() {
	uint32_t a, c = 0x1b;
	asm volatile("rdmsr\n" 
		"or 0x0800, %%eax\n"
		"wrmsr"
		: "=a"(a) : "c"(c));
	
	apic_address = a & 0xfffff000;
}

void apic_write64(int reg, uint64_t v) { 
	*(uint32_t volatile*)(apic_address + reg + 0x10) = (uint32_t)(v >> 32);
	*(uint32_t volatile*)(apic_address + reg) = (uint32_t)v;
}

uint32_t apic_read32(int reg) {
	return *(uint32_t volatile*)(apic_address + reg);
}
