#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>
#include "vnic.h"

VNIC*	manager_nic;

void manager_init();
uint32_t manager_get_ip();
uint32_t manager_set_ip(uint32_t ip);
uint16_t manager_get_port();
void manager_set_port(uint16_t port);
uint32_t manager_get_gateway();
void manager_set_gateway(uint32_t gw);
uint32_t manager_get_netmask();
void manager_set_netmask(uint32_t nm);
void manager_set_interface();

#endif /* __MANAGER_H__ */
