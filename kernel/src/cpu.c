#include <stdio.h>
#include <string.h>
#include "time.h"
#include "port.h"
#include "cpu.h"
#include "asm.h"

char cpu_brand[4 * 4 * 3 + 1];

uint32_t cpu_infos[CPU_INFOS_SIZE][4];
uint32_t cpu_extended_infos[CPU_EXTENDED_INFOS_SIZE][4];

void cpu_init() {
	uint32_t* p = (uint32_t*)cpu_brand;
	
	uint32_t eax = 0x80000002;
	for(int i = 0; i < 12; i += 4) {
		asm volatile("cpuid" 
			: "=a"(p[i + 0]), "=b"(p[i + 1]), "=c"(p[i + 2]), "=d"(p[i + 3]) 
			: "a"(eax++));
	}
	
	// Get CPUID Information
	for(int i = 0; i < CPU_INFOS_SIZE; i++) {
		asm volatile("cpuid"
			: "=a"(cpu_infos[i][0]), "=b"(cpu_infos[i][1]), "=c"(cpu_infos[i][2]), "=d"(cpu_infos[i][3])
			: "a"(i));
	}
	// Get CPUID Extended Information
	for(int i = 0; i < CPU_EXTENDED_INFOS_SIZE; i++) {
		asm volatile("cpuid"
			: "=a"(cpu_extended_infos[i][0]), "=b"(cpu_extended_infos[i][1]), "=c"(cpu_extended_infos[i][2]), "=d"(cpu_extended_infos[i][3])
			: "a"(0x80000000 + i));
	}
}

static void turbo() {
#ifdef _KERNEL_
	// Loader does not have write_msr 
	write_msr(0x00000199, 0xff00);
	printf("\tTurbo Boost Enabled\n");
#endif
}

static void tsc_info() {
	if(CPU_IS_INVARIANT_TSC)
		printf("\tInvariant TSC available\n");
	else
		printf("\tInvariant TSC not available\n");
}

void cpu_info() {
#ifdef _KERNEL_
	printf("\tBrand: %s\n", cpu_brand);
	printf("\tFrequency: %ld\n", cpu_frequency);
#endif

	if(CPU_IS_TURBO_BOOST) {
		printf("\tSupport Turbo Boost Technology\n");
		turbo();
	} else {
		printf("\tNot Support Turbo Boost Technology\n");
	}

	if(cpu_frequency == 0) {
		printf("\tCannot parse CPU frequency...\n");

		while(1)
			asm("hlt");
	}

	tsc_info();
}

