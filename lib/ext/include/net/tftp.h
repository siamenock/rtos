#ifndef __NET_TFTP_H__
#define __NET_TFTP_H__

#include <stdbool.h>
#include <net/packet.h>

#define TFTP_PORT	"net.tftpd.port"
#define TFTP_CALLBACK	"net.tftpd.callback"

typedef struct _TFTPCallback {
	int(*create)(char* filename, int mode);	// 1: read, 2: write
	int (*write)(int fd, void* buf, uint32_t offset, int size);
	int (*read)(int fd, void* buf, uint32_t offset, int size);
} TFTPCallback;

extern uint32_t TFTP_SESSION_GC;
extern uint32_t TFTP_SESSION_TTL;

bool tftp_process(Packet* packet);

#endif /* __NET_TFTP_H__ */
