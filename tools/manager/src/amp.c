#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include "shared.h"

#define __NR_multikernel_boot 326

extern int cpu_start;
extern int cpu_end;
int core_count;

static void wakeup_ap(long kernel_start_address) {
	if(!cpu_end)
		return;

	printf("\tBooting APs : %p\n", kernel_start_address);
	for(int cpu = cpu_start; cpu < cpu_end + 1; cpu++) {
		int apicid = syscall(__NR_multikernel_boot, cpu, kernel_start_address);
		if(apicid < 0) {
			continue;
		}
		shared->mp_cores[apicid] = cpu;
		core_count++;
		printf("\t\tAPIC ID: %d CPU ID: %d\n", apicid, cpu);
	}
	printf("Done\n");
}

void amp_init(long kernel_start_address) {
	wakeup_ap(kernel_start_address);
}

uint8_t get_apic_id() {
	return 0;
}
