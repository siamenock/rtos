#ifndef __NICDEV_H__
#define __NICDEV_H__

#include <net/packet.h>

#define MAX_NIC_NAME_LEN	16

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;

	Map*		vnics;
} NICDevice;

enum NICDEV_PROCESS_RESULT {
	NICDEV_PROCESS_COMPLETE,
	NICDEV_PROCESS_PASS,
};

/**
 * @param dev NIC device
 * @param data data to be sent
 * @param size data size
 *
 * @return result of process
 */
int nicdev_rx(NICDevice* dev, void* data, size_t size);

/**
 * @param dev NIC device
 * @param process function to process packets in NIC device
 * @param context context to be passed to process function
 *
 * @return number of packets proccessed
 */
int nicdev_tx(NICDevice* dev,
		void (*process)(Packet* packet, void* context), void* context);

#endif /* __NICDEV_H__ */
