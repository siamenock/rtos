#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "device.h"
#include "charout.h"
#include "charin.h"

#define SERIAL_IO_ADDR	0x3f8
#define SERIAL_SPEED	115200

DeviceType serial_in_type;
CharIn serial_in_driver;

DeviceType serial_out_type;
CharOut serial_out_driver;

#endif /* __SERIAL_H__ */
