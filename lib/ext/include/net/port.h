#ifndef __NET_PORT_H__
#define __NET_PORT_H__

#include <stdint.h>
#include <nic.h>

uint8_t* port_map_get(NIC* nic, char* key_name);
uint16_t port_alloc(uint8_t* port_table, uint16_t port);
bool port_free(uint8_t* port_table, uint16_t port);

#endif /*__NET_PORT_H__*/
