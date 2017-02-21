#ifndef __VNIC_H__
#define __VNIC_H__

//#include <stdint.h>
#include <linux/types.h>
#include "fifo.h"
#include "list.h"
#include "nic.h"
#include "device.h"

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

bool nic_process_input(NICPriv* priv, uint8_t local_port, uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2);
Packet* nic_process_output(NICPriv* priv, uint8_t local_port);

#endif /* __VNIC_H__ */
