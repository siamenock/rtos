#ifndef __VNIC_DRIVER_H__
#define __VNIC_DRIVER_H__

#include "vnic.h"

/**
 * @file
 * Virtual Network Interface Controller (vNIC) driver API
 */

typedef struct _VNIC_Driver {
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
	VNIC_Queue	input;
	VNIC_Queue	output;
	VNIC_Queue	slow_input;
	VNIC_Queue	slow_output;

	// Malloc pool
	VNIC_Pool	pool;
} VNIC_Driver;

#endif /* __VNIC_DRIVER_H__ */
