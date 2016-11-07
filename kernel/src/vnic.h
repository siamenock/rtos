#ifndef __VNIC_H__
#define __VNIC_H__

#include <stdint.h>
#include <stdbool.h>
#include <util/fifo.h>
#include <util/list.h>
#include <net/nic.h>

#include "device.h"

#define NIC_MAX_DEVICE_COUNT	8

Device* nic_devices[NIC_MAX_DEVICE_COUNT];

typedef struct {
	// Management
	NIC*		nic;
	List*		pools;
	void*		pool;

	Device*		device;
	uint16_t	port;	//port(4) + vlan_id(12)

	// Information
	uint64_t	mac;
	uint64_t	input_bandwidth;
	uint64_t	input_wait;
	uint64_t	input_wait_grace;
	uint64_t	output_bandwidth;
	uint64_t	output_wait;
	uint64_t	output_wait_grace;
	uint16_t	padding_head;
	uint16_t	padding_tail;
	uint16_t	min_buffer_size;
	uint16_t	max_buffer_size;
	List*		input_accept;
	List*		output_accept;

	// Buffer
	uint64_t	input_closed;
	uint64_t	output_closed;
} VNIC;

void vnic_init0();

/**
 * Mandatory attributes to create a NI
 * mac, input_bandwidth, output_bandwidth, input_buffer_size, output_buffer_size, pool_size
 */
Device* nic_parse_index(char* _argv, uint16_t* port);
VNIC* vnic_create(uint64_t* attrs);
void vnic_destroy(VNIC* nic);
bool vnic_contains(char* nic_dev, uint64_t mac);
uint64_t vnic_get_device_mac(char* nic_dev);

/**
 * Return
 * 0: succeed
 * 1: MAC address cannot changable
 * 2: pool size cannot be reduced.
 * 3: pool size must be multiple of 2MBs
 * 4: pool size cannot reduced
 * 5: not enough space
 * 6: no resource to allocate
 */
uint32_t vnic_update(VNIC* nic, uint64_t* attrs);
void nic_process_input(uint8_t local_port, uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2);
Packet* nic_process_output(uint8_t local_port);
void nic_statistics(uint64_t time);

#endif /* __VNIC_H__ */
