#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/time.h>
#include <util/event.h>
#include <util/cmd.h>

#include "command.h"

static bool shell_process(void* context) {
	// Non-block select
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	fd_set temp = *(fd_set*)context;
	int ret = select(STDIN_FILENO + 1, (fd_set*)&temp, NULL, NULL, &tv);

	if(ret == -1) {
		perror("Selector error");

		return false;
	} else if(ret) {
		// FIXME: async control should be refactored not using global flag.
		extern bool cmd_sync;
		if(cmd_sync)
			return true;

		if(FD_ISSET(STDIN_FILENO, (fd_set*)&temp) != 0) {
			// Process input command
			command_process(STDIN_FILENO);
		}
	}

	return true;
}

static bool script_process() {
	int fd = open("./boot.psh", O_RDONLY);
	if(fd == -1)
		return false;

 	command_process(fd);

	return true;
}

bool shell_init() {
 	cmd_init();

	static fd_set stdin;
	FD_ZERO(&stdin);
	FD_SET(STDIN_FILENO, &stdin);

	printf("PacketNgin ver 2.0\n");
	printf("> ");
	fflush(stdout);

	/* Initcial script processing */
	script_process();

	/* Commands input processing */
	event_busy_add(shell_process, &stdin);

	return true;
}
