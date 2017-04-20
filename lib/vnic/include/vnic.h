#ifndef __VNIC_H__
#define __VNIC_H__

#include "nic.h"

/**
 * VNIC attributes to create or update VNIC.
 */
typedef enum {
	VNIC_NONE,			///< End of attributes
	VNIC_ID,			///< NIC ID
	VNIC_MAC,			///< MAC address
	VNIC_DEV,			///< Device of Network Interface
	VNIC_POOL_SIZE,			///< NI's total memory size
	VNIC_RX_BANDWIDTH,		///< Input bandwidth in bps
	VNIC_TX_BANDWIDTH,		///< Output bandwidth in bps
	VNIC_PADDING_HEAD,		///< Minimum padding head of packet payload buffer
	VNIC_PADDING_TAIL,		///< Minimum padding tail of packet payload buffer
	VNIC_RX_QUEUE_SIZE,		///< Number of input buffer size, the packet count
	VNIC_TX_QUEUE_SIZE,	///< Number of output buffer size, the packet count
	VNIC_SLOW_RX_QUEUE_SIZE,
	VNIC_SLOW_TX_QUEUE_SIZE,
	VNIC_MIN_BUFFER_SIZE,		///< Minimum packet payload buffer size which includes padding
	VNIC_MAX_BUFFER_SIZE,		///< Maximum packet payload buffer size which includes padding
	VNIC_RX_ACCEPT_ALL,		///< To accept all packets to receive
	VNIC_RX_ACCEPT,		///< List of accept MAC addresses to receive
	VNIC_TX_ACCEPT_ALL,		///< To accept all packets to send
	VNIC_TX_ACCEPT,		///< List of accept MAC addresses to send
} VNIC_ATTRIBUTES;

typedef struct {
	// Management
	NIC*		nic;
	uint32_t	nic_size;
	char		parent[MAX_NIC_NAME_LEN];

	// Information
	uint64_t	magic;

	uint32_t	id;	// NIC unique ID (unique ID in RTOS)

	uint64_t	mac;

	uint64_t	rx_bandwidth;
	uint64_t	tx_bandwidth;

	uint16_t	padding_head;
	uint16_t	padding_tail;

	NIC_Queue	rx;
	uint64_t	rx_wait;
	uint64_t	rx_wait_grace;

	NIC_Queue	tx;
	uint64_t	tx_wait;
	uint64_t	tx_wait_grace;

	NIC_Queue	srx;
	NIC_Queue	stx;

	// Buffer
	uint64_t	rx_closed;
	uint64_t	tx_closed;
} VNIC;

bool vnic_init(VNIC* vnic, uint64_t* attrs);
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
