#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <util/event.h>
#include <termios.h>
#include "input.h"
#include "../stdio.h"

static bool output_enabled;
static CharInCallback callback;
static int init(void* device, void* data) {
	if(output_enabled)
		return 0; // Already enabled 

	struct termios termios;
	tcgetattr(STDIN_FILENO, &termios);
	termios.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &termios);

	output_enabled = true;
	return 0;
}

static void destroy(int id) {
	system("stty echo");
}

static int _write(int id, const char* buf, int len) {
	if(!output_enabled)
		return -1;

	while(len) {
		//FIXME stdout bug???
		//Can't flush stdout buffer
		len -= write(STDOUT_FILENO, buf, len);
	}

	return 0;
}

static int scroll(int id, int lines) {
	//TODO
	return 0;
}

static void set_cursor(int id, int rows, int cols) {
	//TODO
}

static void get_cursor(int id, int* rows, int* cols) {
	//TODO
}

static void set_render(int id, int is_render) {
	//TODO
}

static int is_render(int id) {
	//TODO
	return 0;
}

DeviceType output_type = DEVICE_TYPE_CHAR_OUT;

CharOut output_driver = {
	.init = init,
	.destroy = destroy,
	.write = _write,
	.scroll = scroll,
	.set_cursor = set_cursor,
	.get_cursor = get_cursor,
	.set_render = set_render,
	.is_render = is_render
};

static bool event(void* context) {
	// Non-block select
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	char a;

	fd_set temp;
	fsync(STDIN_FILENO);
	FD_ZERO(&temp);
	FD_SET(STDIN_FILENO, &temp);
	int ret = select(STDIN_FILENO + 1, (fd_set*)&temp, NULL, NULL, &tv);
	if(ret == -1) {
		perror("Selector error");
		return false;
	} else if(ret) {
		if(FD_ISSET(STDIN_FILENO, (fd_set*)&temp) != 0) {
			read(STDIN_FILENO, &a, 1);
			stdio_putchar(a);
		}
	}

	if(callback)
		callback();

	return true;
}

static void set_callback(int id, CharInCallback cb) {
	callback = cb;
	event_timer_add(event, NULL, 100000, 100000);
}

DeviceType input_type = DEVICE_TYPE_CHAR_IN;

CharIn input_driver = {
	.init = init,
	.destroy = destroy,
	.set_callback = set_callback
};
