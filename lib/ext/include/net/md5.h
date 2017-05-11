#ifndef __NET_MD5_H__
#define __NET_MD5_H__

#include <stdint.h>

/**
 * @file
 * Message Digest version 5
 */

/**
 * Calculate MD5 digest
 *
 * @param message message body
 * @param len message length
 * @param[out] hash 4 bytes length hash pointer (it must be preallocated)
 */
void md5(uint8_t* message, uint32_t len, uint32_t* hash);

/**
 * Multiple memory blocks MD5 digesting
 *
 * @param blocks block array
 * @param block_count block array count
 * @param block_size each block size, it must be multiple of 64 bytes
 * @param len total length to calculate MD5 digest
 * @param[out] hash 4 bytes length hash pointer (it must be preallocated)
 */
void md5_blocks(void** blocks, uint32_t block_count, uint32_t block_size, uint64_t len, uint32_t* hash);

#endif /* __NET_MD5_H__ */
