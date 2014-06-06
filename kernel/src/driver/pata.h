#ifndef __PATA_H__
#define __PATA_H__

#include "../device.h"

bool pata_available(uint32_t no);
Device* pata_device();

#endif /* __PATA_H__ */
