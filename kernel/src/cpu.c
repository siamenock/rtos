#include <stdio.h>
#include <string.h>
#include <timer.h>
#include <util/cmd.h>
#include "port.h"
#include "cpu.h"
#include "msr.h"

char cpu_brand[4 * 4 * 3 + 1];

static int cmd_turbo(int argc, char** argv, void(*callback)(char* result, int exit_status));
static Command commands[] = {
	{
		.name = "turbo",
		.desc = "Manage Turbo Boost.\n"
			"If the switch argument is not given, the turbo boost status is printed.\n"
			"otherwise, turbo boost will be enabled/disabled.",
		.args = "[switch: str{on|off}]",
		.func = cmd_turbo
	},
};

void cpu_init() {
	uint32_t* p = (uint32_t*)cpu_brand;

	uint32_t eax = 0x80000002;
	for(int i = 0; i < 12; i += 4) {
		asm volatile("cpuid"
			: "=a"(p[i + 0]), "=b"(p[i + 1]), "=c"(p[i + 2]), "=d"(p[i + 3])
			: "a"(eax++));
	}

	printf("\tBrand: %s\n", cpu_brand);

	bool has_turbo_boost = cpu_has_feature(CPU_FEATURE_TURBO_BOOST);
	printf("\tTurbo boost: %s\n", has_turbo_boost ? "\x1b""32msupported""\x1b""0m" : "not supported");
	if(has_turbo_boost) {
		msr_write(0xff00, MSR_IA32_PERF_CTL);
		printf("\tTurbo boost: ""\x1b""32menabled""\x1b""0m\n");
	}

	bool has_invariant_tsc = cpu_has_feature(CPU_FEATURE_INVARIANT_TSC);
	printf("\tInvariant TSC: %s\n", has_invariant_tsc ? "\x1b""32msupported""\x1b""0m" : "\x1b""31mnot supported""\x1b""0m");

	cmd_register(commands, sizeof(commands) / sizeof(commands[0]));
}

bool cpu_has_feature(int feature) {
	uint32_t a, b, c, d;

	#define INFO(cmd) asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"((cmd)))
	#define EXT(cmd) asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0x80000000 + (cmd)))

	switch(feature) {
		case CPU_FEATURE_SSE_4_1:
			INFO(0x01);
			return !!(c & 0x80000);
		case CPU_FEATURE_SSE_4_2:
			INFO(0x01);
			return !!(c & 0x100000);
		case CPU_FEATURE_MONITOR_MWAIT:
			INFO(0x01);
			return !!(c & 0x8);
		case CPU_FEATURE_MWAIT_INTERRUPT:
			INFO(0x05);
			return !!(c & 0x2);
		case CPU_FEATURE_TURBO_BOOST:
			INFO(0x06);
			return !!(a & 0x2);
		case CPU_FEATURE_INVARIANT_TSC:
			EXT(0x07);
			return !!(d & 0x100);
		default:
			return false;
	}
}

static int cmd_turbo(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc != 1 && argc != 2)
		return CMD_STATUS_WRONG_NUMBER;

	uint64_t perf_status = msr_read(MSR_IA32_PERF_STATUS);
	perf_status = ((perf_status & 0xff00) >> 8);
	if(!cpu_has_feature(CPU_FEATURE_TURBO_BOOST)) {
		printf("Not Support Turbo Boost\n");
		printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);

		return 0;
	}

	uint64_t perf_ctrl = msr_read(MSR_IA32_PERF_CTL);

	if(argc == 1) {
		if(perf_ctrl & 0x100000000) {
			printf("Turbo Boost Disabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		} else {
			printf("Turbo Boost Enabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}

		return 0;
	}

	if(!strcmp(argv[1], "off")) {
		if(perf_ctrl & 0x100000000) {
			printf("Turbo Boost Already Disabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n",  perf_status / 10, perf_status % 10);
		} else {
			msr_write(0x10000ff00, MSR_IA32_PERF_CTL);
			printf("Turbo Boost Disabled\n");
			perf_status = msr_read(MSR_IA32_PERF_STATUS);
			perf_status = ((perf_status & 0xff00) >> 8);
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}
	} else if(!strcmp(argv[1], "on")) {
		if(perf_ctrl & 0x100000000) {
			msr_write(0xff00, MSR_IA32_PERF_CTL);
			printf("Turbo Boost Enabled\n");
			perf_status = msr_read(MSR_IA32_PERF_STATUS);
			perf_status = ((perf_status & 0xff00) >> 8);
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		} else {
			printf("Turbo Boost Already Enabled\n");
			printf("\tPerfomance status : %d.%d Ghz\n", perf_status / 10, perf_status % 10);
		}
	} else {
		return -1;
	}

	return 0;
}
