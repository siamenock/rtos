#include <string.h>
#include <errno.h>
#include <util/event.h>
#include <util/ring.h>
#include <util/list.h>
#include "../port.h"
#include "../apic.h"
#include "../stdio.h"
#include "keyboard.h"
#include "usb/usb.h"

static CharInCallback callback;

static bool event(void* data) {
	if(usb_keyboard) 
		usb_hid_poll(usb_keyboard);
	
	if(callback)
		callback();

	return true;
}

static void keyboard_handler(uint64_t vector, uint64_t error_code) {
	char code = port_in8(0x60);
	stdio_scancode((uint8_t)code); 
	apic_eoi();
}

static int init(void* device, void* data) {
	//apic_register(32 + 1, keyboard_handler);
	return 0;
}

static void destroy(int id) {
	apic_register(32 + 1, NULL);
}

static void set_callback(int id, CharInCallback cb) {
	port_in8(0x60);	// Empty key buffer
	
	callback = cb;
	event_timer_add(event, NULL, 100000, 100000);
}

DeviceType keyboard_type = DEVICE_TYPE_CHAR_IN;

CharIn keyboard_driver = {
	.init = init,
	.destroy = destroy,
	.set_callback = set_callback
};
