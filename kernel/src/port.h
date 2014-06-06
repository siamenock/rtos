#ifndef __PORT_H__
#define __PORT_H__

#include <stdint.h>

uint8_t port_in8(uint16_t port);
void port_out8(uint16_t port, uint8_t data);

uint16_t port_in16(uint16_t port);
void port_out16(uint16_t port, uint16_t data);

uint32_t port_in32(uint16_t port);
void port_out32(uint16_t port, uint32_t data);

#endif /* __PORT_H__ */
