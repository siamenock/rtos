#include "msr.h"

uint64_t msr_read(uint32_t register_address) {
	/* Reads the contents of a 64-bit model specific register */
	/* specified in the ECX register into registers EDX:EAX. */
	uint64_t cx = register_address;
	uint64_t ax;
	uint64_t dx;
 	asm volatile("rdmsr" : "=a"(ax), "=d"(dx) : "c"(cx));
 	return (dx << 32) | ax;
}

void msr_write(uint64_t value, uint32_t register_address) {
	msr_write2(value >> 32, value & 0xFFFFFFFF, register_address);
}

void msr_write2(uint32_t dx, uint32_t ax, uint32_t register_address) {
	asm volatile("wrmsr" : : "d"(dx), "a"(ax), "c"(register_address));
}
