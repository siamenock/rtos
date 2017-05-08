#ifndef __DRIVER_CONSOLE_H__
#define __DRIVER_CONSOLE_H__

#include "device.h"
#include "charout.h"
#include "charin.h"

/* Console output driver */
enum {
    CONSOLE_VGA_DRIVER,
    CONSOLE_SERIAL_OUT_DRIVER,
    CONSOLE_OUTPUT_DRIVER,
     
    CONSOLE_OUT_DRIVERS_COUNT
};

/* Console input driver */
enum {
    CONSOLE_KEYBOARD_DRIVER,
    CONSOLE_SERIAL_IN_DRIVER,
    CONSOLE_INPUT_DRIVER,
     
    CONSOLE_IN_DRIVERS_COUNT
};

void console_init();

DeviceType console_out_type;
CharOut console_out_driver;

DeviceType console_in_type;
CharIn console_in_driver;

#endif /* __DRIVER_CONSOLE_H__ */
