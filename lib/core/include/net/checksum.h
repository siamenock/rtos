#ifndef __NET_CHECKSUM_H__
#define __NET_CHECKSUM_H__

#include <stdint.h>

/**
 * @file
 * IPv4 checksum
 */

/**
 * Calculate IPv4 header checksum
 *
 * @param data header data
 * @param size header size
 * @return checksum
 */
uint16_t checksum(void* data, uint32_t size);

#endif /* __NET_CHECKSUM_H__ */
