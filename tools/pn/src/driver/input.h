#ifndef __INPUT_H__
#define __INPUT_H__

#include "device.h"
#include "charin.h"
#include "charout.h"
/*
   * Character I/O Device for Linux
 */

DeviceType output_type;
CharOut output_driver;

DeviceType input_type;
CharIn input_driver;

#endif /* __INPUT_H__ */
