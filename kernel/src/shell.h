#ifndef __SHELL_H__
#define __SHELL_H__

#include <packet.h>

#define CMD_SIZE        512

void shell_init();
bool shell_process(Packet* packet);

#endif /* __SHELL_H__ */
