#ifndef __NET_CRC_H__
#define __NET_CRC_H__

#include <stdint.h>

/**
 * @file
 * Calculate 32 bits cyclic redundancy checks
 */

/**
 * Calculate CRC32
 *
 * @param data message
 * @param len message length
 * @return CRC32
 */
uint32_t crc32(uint8_t* data, uint32_t len);

/**
 * Update CRC32 value
 *
 * @param crc previously calculated crc32
 * @param data message
 * @param len message length
 * @return CRC32
 */
uint32_t crc32_update(uint32_t crc, uint8_t* data, uint32_t len);

#endif /* __NET_CRC_H__ */
