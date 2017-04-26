#ifndef __NICDEV_H__
#define __NICDEV_H__

#include <vnic.h>

#define MAX_NIC_DEVICE_COUNT	128
#define MAX_NIC_NAME_LEN	16

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;
	void*		driver;

	VNIC*		vnics[MAX_VNIC_COUNT];
} NICDevice;

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;
} NICInfo;

typedef struct {
	int		mtu;
} NICStatus;

typedef struct {
	int		(*init)(void* device, void* data);
	void		(*destroy)(int id);
	int		(*poll)(int id);
	void		(*get_status)(int id, NICStatus* status);
	bool		(*set_status)(int id, NICStatus* status);
	void		(*get_info)(int id, NICInfo* info);
} NICDriver;

int nicdev_register(NICDevice* dev);
NICDevice* nicdev_unregister(const char* name);
NICDevice* nicdev_unregister(const char* name);
NICDevice* nicdev_get(const char* name);
int nicdev_poll();

uint32_t nicdev_register_vnic(NICDevice* dev, VNIC* vnic);
VNIC* nicdev_unregister_vnic(NICDevice* dev, uint32_t id);
VNIC* nicdev_get_vnic(NICDevice* dev, uint32_t id);
VNIC* nicdev_get_vnic_mac(NICDevice* dev, uint64_t mac);
VNIC* nicdev_update_vnic(NICDevice* dev, VNIC* src_vnic);

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
		bool (*process)(Packet* packet, void* context), void* context);

#endif /* __NICDEV_H__ */
