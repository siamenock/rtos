#ifndef __TUN_H__
#define __TUN_H__

#include <linux/if_tun.h>
#include <util/fifo.h>
#include <util/types.h>

#ifndef IFNAMSIZE
#define IFNAMSIZ 		16	
#endif
#define MAX_BUFFER_SIZE 1500

typedef struct _TunInterface {
	// Basic information 
	int fd;
	char name[IFNAMSIZ];

	// Information	
	uint64_t mac;
	uint16_t min_buffer_size;
	uint16_t max_buffer_size;

	// Lock
	volatile uint8_t input_lock;
	volatile uint8_t output_lock;

	// Buffer 
	char buffer[MAX_BUFFER_SIZE];
	FIFO* input_buffer;
	FIFO* output_buffer;
} TunInterface;	

void tun_init(void);
TunInterface* tun_create(const char* dev, int flags);
void tun_destroy(TunInterface* ti);
//char* ti_input(TunInterface* ti); // TODO : char* -> Packet* change
//bool ti_output(TunInterface* ti, char* buffer); // TODO : same 

#endif /* __TUN_H__*/ 
