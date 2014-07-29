#ifndef __SHELL_H__
#define __SHELL_H__

#include <net/packet.h>

void shell_init();
bool shell_process(Packet* packet);

#endif /* __SHELL_H__ */
