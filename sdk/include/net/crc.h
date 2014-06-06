#ifndef __NET_CRC_H__
#define __NET_CRC_H__

#include <stdint.h>

uint32_t crc32(uint8_t* data, uint32_t len);
uint32_t crc32_update(uint32_t crc, uint8_t* data, uint32_t len);

#endif /* __NET_CRC_H__ */
