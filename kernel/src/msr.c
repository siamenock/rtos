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

void msr_write(uint32_t dx, uint32_t ax, uint32_t register_address) {
	asm volatile("wrmsr" : : "d"(dx), "a"(ax), "c"(register_address));
}
