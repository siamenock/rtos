#ifndef __NIC_DRIVER_H__
#define __NIC_DRIVER_H__

#include "nic.h"

/**
 * @file
 * Virtual Network Interface Controller (vNIC) driver API
 */

typedef struct _NIC_Driver {
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

	// Buffer
	uint64_t	input_closed;
	uint64_t	output_closed;

	// Queue
	NIC_Queue	input;
	NIC_Queue	output;
	NIC_Queue	slow_input;
	NIC_Queue	slow_output;

	// Malloc pool
	NIC_Pool	pool;
} NIC_Driver;

#endif /* __NIC_DRIVER_H__ */
