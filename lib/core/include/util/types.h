#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>
#include <stdbool.h>

bool is_uint8(const char* val);
uint8_t parse_uint8(const char* val);
bool is_uint16(const char* val);
uint16_t parse_uint16(const char* val);
bool is_uint32(const char* val);
uint32_t parse_uint32(const char* val);
bool is_uint64(const char* val);
uint64_t parse_uint64(const char* val);

#endif /* __TYPES_H__ */
