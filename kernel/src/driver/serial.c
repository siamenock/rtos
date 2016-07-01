#include <stdbool.h>
#include <string.h>
#include <util/event.h>
#include "../port.h"
#include "../stdio.h"
#include "serial.h"
#include "charin.h"

static bool serial_enabled;
static CharInCallback callback;

/* Functions related to both serial console output/intput device driver */ 
static int init(void* device, void* data) {
	if(serial_enabled)
		return 0; // Already enabled 

	if((port_in8(SERIAL_IO_ADDR + 0x05) == 0xFF) 
			&& (port_in8(SERIAL_IO_ADDR + 0x06) == 0xFF)) {
		serial_enabled = false;
		return -1;
	}

	/* Disable interrupts. */
	port_out8(SERIAL_IO_ADDR + 0x01, 0);

	/* Assert RTS and DTR. */
	port_out8(SERIAL_IO_ADDR + 0x04, 3);

	/* Set the Line control register to user divisor latch. */
	uint8_t reg = port_in8(SERIAL_IO_ADDR + 0x03);
	port_out8(SERIAL_IO_ADDR + 0x03, reg | 0x80);

	/* Set the divisor latch for speed. */
	int speed = SERIAL_SPEED; // Full speed
	uint16_t divisor = 115200 / speed;
	port_out8(SERIAL_IO_ADDR + 0x00, divisor & 0xFF);
	port_out8(SERIAL_IO_ADDR + 0x01, divisor >> 8);

	/* Restore the previous value of the divisor.   *
	 * And set 8 bits per character			*/
	port_out8(SERIAL_IO_ADDR + 0x03, (reg & ~0x80));

	serial_enabled = true;
	return 0;
}

static void destroy(int id) {
	// Do nothing
}

/* Functions related to serial console output device driver */
static void serial_putchar(uint8_t ch) {
	while((port_in8(SERIAL_IO_ADDR + 0x05) & 0x20) == 0); // Wait till empty

	port_out8(SERIAL_IO_ADDR, ch);
	if(ch == '\n')
		serial_putchar('\r');
}

static int write(int id, const char* buf, int len) {
	if(!serial_enabled)
		return -1;

	for(int i = 0; i < len; i++) 
		serial_putchar(buf[i]);

	return 0;
}

static int scroll(int id, int lines) {
	return 0;
}

static void set_cursor(int id, int rows, int cols) {
}

static void get_cursor(int id, int* rows, int* cols) {
}

static void set_render(int id, int is_render) {
}

static int is_render(int id) {
	return 0;
}

DeviceType serial_out_type = DEVICE_TYPE_CHAR_OUT;

CharOut serial_out_driver = {
	.init = init,
	.destroy = destroy,
	.write = write,
	.scroll = scroll,
	.set_cursor = set_cursor,
	.get_cursor = get_cursor,
	.set_render = set_render,
	.is_render = is_render
};

/* Functions related to serial console input device driver */
static int serial_havechar() {
	return port_in8(SERIAL_IO_ADDR + 0x05) & 0x01;
}

static int serial_getchar() {
	return port_in8(SERIAL_IO_ADDR);
}

static bool event(void* data) {
	int ch;
	if(serial_enabled) {
		while(serial_havechar()) {
			ch = serial_getchar();
			// TODO: process other special keys
			if(ch == '\r') {
				// Serial mode of QEMU is cooked mode. 
				// Line feed('\r') expected for new line('\n') 
				stdio_putchar((const char)'\n');
			} else {
				stdio_putchar((const char)ch);
			}

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

DeviceType serial_in_type = DEVICE_TYPE_CHAR_IN;

CharIn serial_in_driver = {
	.init = init,
	.destroy = destroy,
	.set_callback = set_callback
};
