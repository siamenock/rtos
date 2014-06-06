#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <stdint.h>

void manager_init();
void manager_get_ip(uint32_t* ip, uint32_t* netmask, uint32_t* gateway);
void manager_set_ip(uint32_t ip, uint32_t netmask, uint32_t gateway);

#endif /* __MANAGER_H__ */
