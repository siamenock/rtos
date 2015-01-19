#include <string.h>
#include <errno.h>
#include <util/event.h>
#include <util/ring.h>
#include <util/list.h>
#include "../port.h"
#include "../apic.h"
#include "keyboard.h"

static CharInCallback callback;

#define KEYBOARD_BUFFER_SIZE	8

static char buffer[8];
static size_t head;
static size_t tail;

static bool event(void* data) {
	int code = 0;

	while(ring_read(buffer, &head, tail, KEYBOARD_BUFFER_SIZE, (char*)&code, 1)) {
		if(callback)
			callback((int)code);
	}

	return true;
}

static void keyboard_handler(uint64_t vector, uint64_t error_code) {
	char code = port_in8(0x60);
	ring_write(buffer, head, &tail, KEYBOARD_BUFFER_SIZE, &code, 1);
	apic_eoi();
}

static int init(void* device, void* data) {
	apic_register(32 + 1, keyboard_handler);
	
	/*
	int i, j;
	port_out8(0x64, 0xae);	// Activate keyboard controller
	for(i = 0; i < 0xffff; i++) {	// Try to wait until input buffer is empty
		if(!(port_in8(0x64) & 0x02)) {
			break;
		}
	}
	
	port_out8(0x60, 0xf4);	// Activate keyboard
	for(j = 0; j < 100; j++) {
		for(i = 0; i < 0xffff; i++) {	// Try to wait until output buffer is not empty
			if(port_in8(0x64) & 0x01) {
				break;
			}
		}
		if(port_in8(0x60) == 0xfa) {	// Check keyboard ACK
			return 0;
		}
	}
	*/
	//errno = 1;	// Cannot initialize keyboard
	
	// TODO: Fix it
	return 0;
}

static void destroy(int id) {
	apic_register(32 + 1, NULL);
}

static void set_callback(int id, CharInCallback cb) {
	callback = cb;
	event_timer_add(event, NULL, 100000, 100000);
}

DeviceType keyboard_type = DEVICE_TYPE_CHAR_IN;

CharIn keyboard_driver = {
	.init = init,
	.destroy = destroy,
	.set_callback = set_callback
};
