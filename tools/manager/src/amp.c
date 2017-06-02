#include <stdio.h>
#include <unistd.h>
#include <syscall.h>
#include "mmap.h"
#include "shared.h"

#define __NR_multikernel_boot 326

extern int cpu_start;
extern int cpu_end;
int core_count;

static void wakeup_ap(long kernel_start_address) {
	if(!cpu_end)
		return;

	Shared* shared = (Shared*)SHARED_ADDR;
	printf("\tBooting APs : %p\n", (void*)(uintptr_t)kernel_start_address);
	for(int cpu = cpu_start; cpu < cpu_end + 1; cpu++) {
		int apicid = syscall(__NR_multikernel_boot, cpu, kernel_start_address);
		if(apicid < 0) {
			continue;
		}
		shared->mp_processors[apicid] = cpu;
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
