#ifndef __NICDEV_H__
#define __NICDEV_H__

#include <vnic.h>

#define MAX_NIC_DEVICE_COUNT	128
#define MAX_NIC_NAME_LEN	16

typedef struct {
	char		name[MAX_NIC_NAME_LEN];
	uint64_t	mac;
	void*		driver;
	void*		priv;

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
	void		(*destroy)(void* priv);
	int		(*poll)(void* priv);
	void		(*get_status)(void* priv, NICStatus* status);
	bool		(*set_status)(void* priv, NICStatus* status);
	void		(*get_info)(void* priv, NICInfo* info);
} NICDriver;

int nicdev_get_count();
NICDevice* nicdev_create();
void nicdev_destroy(NICDevice* dev);
int nicdev_register(NICDevice* dev);
NICDevice* nicdev_unregister(const char* name);
NICDevice* nicdev_unregister(const char* name);
NICDevice* nicdev_get_by_idx(int idx);
NICDevice* nicdev_get(const char* name);
int nicdev_poll();
void nicdev_free(Packet* packet);

int nicdev_register_vnic(NICDevice* dev, VNIC* vnic);
VNIC* nicdev_unregister_vnic(NICDevice* dev, uint32_t id);
VNIC* nicdev_get_vnic(NICDevice* dev, uint32_t id);
VNIC* nicdev_get_vnic_mac(NICDevice* dev, uint64_t mac);
VNIC* nicdev_update_vnic(NICDevice* dev, VNIC* src_vnic);

enum NICDEV_PROCESS_RESULT {
	NICDEV_PROCESS_COMPLETE,
	NICDEV_PROCESS_PASS,
};

#define	NICDEV_DEBUG_OFF			0
#define	NICDEV_DEBUG_PACKET_INFO		1 << 1
#define	NICDEV_DEBUG_PACKET_DUMP		1 << 2
#define NICDEV_DEBUG_PACKET_ETHER_INFO		1 << 3
#define NICDEV_DEBUG_PACKET_VERBOSE_INFO	1 << 4

/**
 * @param debug option
 */
void nidev_debug_switch_set(uint8_t opt);

/**
 * @return debug option
 */
uint8_t nicdev_debug_switch_get();

/**
 * @param dev NIC device
 * @param data data to be sent
 * @param size data size
 *
 * @return result of process
 */
int nicdev_rx(NICDevice* dev, void* data, size_t size);

/**
 * @param dev NIC Device
 * @param data data to be sent
 * @param size data size
 * @param data_optional optional data to be sent
 * @param size_optional opttional data size
 *
 * @return  result of process
 */
int nicdev_rx0(NICDevice* dev, void* data, size_t size, void* data_optional, size_t size_optional);

/**
 * @param dev NIC device
 * @param process function to process packets in NIC device
 * @param context context to be passed to process function
 *
 * @return number of packets proccessed
 */
int nicdev_tx(NICDevice* dev, bool (*process)(Packet* packet, void* context), void* context);

#endif /* __NICDEV_H__ */
