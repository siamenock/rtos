#ifndef __VNIC_H__
#define __VNIC_H__

#include "nic.h"

/**
 * VNIC attributes to create or update VNIC.
 */
typedef enum {
	VNIC_NONE,			///< End of attributes
	VNIC_MAC,			///< MAC address
	VNIC_DEV,			///< Device of Network Interface
	VNIC_POOL_SIZE,			///< NI's total memory size
	VNIC_INPUT_BANDWIDTH,		///< Input bandwidth in bps
	VNIC_OUTPUT_BANDWIDTH,		///< Output bandwidth in bps
	VNIC_PADDING_HEAD,		///< Minimum padding head of packet payload buffer
	VNIC_PADDING_TAIL,		///< Minimum padding tail of packet payload buffer
	VNIC_INPUT_BUFFER_SIZE,		///< Number of input buffer size, the packet count
	VNIC_OUTPUT_BUFFER_SIZE,	///< Number of output buffer size, the packet count
	VNIC_SLOW_INPUT_BUFFER_SIZE,
	VNIC_SLOW_OUTPUT_BUFFER_SIZE,
	VNIC_MIN_BUFFER_SIZE,		///< Minimum packet payload buffer size which includes padding
	VNIC_MAX_BUFFER_SIZE,		///< Maximum packet payload buffer size which includes padding
	VNIC_INPUT_ACCEPT_ALL,		///< To accept all packets to receive
	VNIC_INPUT_ACCEPT,		///< List of accept MAC addresses to receive
	VNIC_OUTPUT_ACCEPT_ALL,		///< To accept all packets to send
	VNIC_OUTPUT_ACCEPT,		///< List of accept MAC addresses to send
} VNIC_ATTRIBUTES;

typedef struct {
	// Management
	NIC*		nic;

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

VNIC* vnic_create(uint64_t* attrs);
void vnic_destroy(VNIC* nic);
uint32_t vnic_update(VNIC* nic, uint64_t* attrs);

bool vnic_has_rx(VNIC* vnic);
bool vnic_rx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_rx2(VNIC* vnic, Packet* packet);

bool vnic_has_srx(VNIC* vnic);
bool vnic_srx(VNIC* vnic, uint8_t* buf1, size_t size1, uint8_t* buf2, size_t size2);
bool vnic_srx2(VNIC* vnic, Packet* packet);

bool vnic_has_tx(VNIC* vnic);
Packet* vnic_tx(VNIC* vnic);

bool vnic_has_stx(VNIC* vnic);
Packet* vnic_stx(VNIC* vnic);

#endif /* __VNIC_H__ */

