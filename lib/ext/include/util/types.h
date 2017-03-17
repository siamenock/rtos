#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @file
 * Basic utility to identify the string is interger or not
 */

/**
 * Check the string is 8-bits interger.
 *
 * @param val a string
 * @return true if the string is 8-bits integer.
 */
bool is_uint8(const char* val);

/**
 * Parse a string to 8-bits integer.
 *
 * @param val a string
 * @return parsed integer
 */
uint8_t parse_uint8(const char* val);

/**
 * Check the string is 16-bits interger.
 *
 * @param val a string
 * @return true if the string is 16-bits integer.
 */
bool is_uint16(const char* val);

/**
 * Parse a string to 16-bits integer.
 *
 * @param val a string
 * @return parsed integer
 */
uint16_t parse_uint16(const char* val);

/**
 * Check the string is 32-bits interger.
 *
 * @param val a string
 * @return true if the string is 32-bits integer.
 */
bool is_uint32(const char* val);

/**
 * Parse a string to 32-bits integer.
 *
 * @param val a string
 * @return parsed integer
 */
uint32_t parse_uint32(const char* val);

/**
 * Check the string is 64-bits interger.
 *
 * @param val a string
 * @return true if the string is 64-bits integer.
 */
bool is_uint64(const char* val);

/**
 * Parse a string to 64-bits integer.
 *
 * @param val a string
 * @return parsed integer
 */
uint64_t parse_uint64(const char* val);

#endif /* __TYPES_H__ */
