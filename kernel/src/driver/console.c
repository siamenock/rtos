#include <stdbool.h>
#include <string.h>
#include "console.h"
#include "vga.h"
#include "keyboard.h"
#include "serial.h"

static CharOut* console_out_drivers[CONSOLE_OUT_DRIVERS_COUNT];
static CharIn*	console_in_drivers[CONSOLE_IN_DRIVERS_COUNT];

#define EACH_OUT_CONSOLES(FUNC, ...)				\
	for(int i = 0; i < CONSOLE_OUT_DRIVERS_COUNT; i++)	\
		console_out_drivers[i]->FUNC(__VA_ARGS__);

#define EACH_IN_CONSOLES(FUNC, ...)				\
	for(int i = 0; i < CONSOLE_IN_DRIVERS_COUNT; i++)	\
		console_in_drivers[i]->FUNC(__VA_ARGS__);

void console_init() {
	console_out_drivers[CONSOLE_VGA_DRIVER] = &vga_driver;
	console_out_drivers[CONSOLE_SERIAL_OUT_DRIVER] = &serial_out_driver;

	console_in_drivers[CONSOLE_VGA_DRIVER] = &keyboard_driver;
	console_in_drivers[CONSOLE_SERIAL_IN_DRIVER] = &serial_in_driver;
}

static int console_out_init(void* device, void* data) {
	EACH_OUT_CONSOLES(init, device, data);

	return 0;
}

static void console_out_destroy(int id) {
	EACH_OUT_CONSOLES(destroy, id);
}

static int write(int id, const char* buf, int len) {
	EACH_OUT_CONSOLES(write, id, buf, len);

	return 0;
}

static int scroll(int id, int lines) {
	EACH_OUT_CONSOLES(scroll, id, lines);

	return 0;
}

static void set_cursor(int id, int rows, int cols) {
	EACH_OUT_CONSOLES(set_cursor, id, rows, cols);
}

static void get_cursor(int id, int* rows, int* cols) {
	EACH_OUT_CONSOLES(get_cursor, id, rows, cols);
}

static void set_render(int id, int is_render) {
	EACH_OUT_CONSOLES(set_render, id, is_render);
}

static int is_render(int id) {
	return 0;
}

DeviceType console_out_type = DEVICE_TYPE_CHAR_OUT;

CharOut console_out_driver = {
	.init = console_out_init,
	.destroy = console_out_destroy,
	.write = write,
	.scroll = scroll,
	.set_cursor = set_cursor,
	.get_cursor = get_cursor,
	.set_render = set_render,
	.is_render = is_render
};

static int console_in_init(void* device, void* data) {
	EACH_IN_CONSOLES(init, device, data);

	return 0;
}

static void console_in_destroy(int id) {
	EACH_IN_CONSOLES(destroy, id);
}

static void set_callback(int id, CharInCallback cb) {
	EACH_IN_CONSOLES(set_callback, id, cb);
}

DeviceType console_in_type = DEVICE_TYPE_CHAR_IN;

CharIn console_in_driver = {
	.init = console_in_init,
	.destroy = console_in_destroy,
	.set_callback = set_callback
};
