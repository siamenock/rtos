#include <stdio.h>
#include <util/cmd.h>

#include "version.h"

static int print_version(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
//TODO fix arguments parse
	printf("PacketNgin %s\n", VERSION);

	return 0;
}

static Command commands[] = {
	{
		.name = "version",
		.desc = "Print the kernel version.",
		.func = print_version
	},
};

int ver_init() {
	print_version(0, NULL, NULL);
	printf("# ");

	if(cmd_register(commands, sizeof(commands) / sizeof(commands[0]))) return -1;

	return 0;
}
