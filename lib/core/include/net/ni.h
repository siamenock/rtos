#ifndef __NET_NETWORK_INTERFACE_H__
#define __NET_NETWORK_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <net/packet.h>
#include <util/fifo.h>
#include <util/map.h>

#define NI_NONE			0
#define NI_MAC			1
#define NI_POOL_SIZE		2
#define NI_INPUT_BANDWIDTH	3
#define NI_OUTPUT_BANDWIDTH	4
#define NI_PADDING_HEAD		5
#define NI_PADDING_TAIL		6
#define NI_INPUT_BUFFER_SIZE	7
#define NI_OUTPUT_BUFFER_SIZE	8
#define NI_MIN_BUFFER_SIZE	9
#define NI_MAX_BUFFER_SIZE	10
#define NI_INPUT_ACCEPT_ALL	11
#define NI_INPUT_ACCEPT		12
#define NI_OUTPUT_ACCEPT_ALL	13
#define NI_OUTPUT_ACCEPT	14
#define NI_INPUT_FUNC		15

#define NIS_SIZE		256

#define PACKET_STATUS_IP		(1 >> 0)	// Is IP(v4, v6) packet?
#define PACKET_STATUS_IPv6		(1 >> 1)	// Is IPv6 packet?
#define PACKET_STATUS_UDP		(1 >> 2)	// Is UDP packet?
#define PACKET_STATUS_TCP		(1 >> 3)	// Is TCP packet?
#define PACKET_STATUS_L2_CHECKSUM	(1 >> 4)	// Is level 2(IP, IPv6) RX checksum OK or TX checksum already calculated
#define PACKET_STATUS_L3_CHECKSUM	(1 >> 5)	// Is level 3(TCP, UDP) RX checksum OK or TX checksum already calculated

typedef struct _NetworkInterface NetworkInterface;

typedef struct _NetworkInterface {
	// Management
	uint64_t	pool_size;
	void*		pool;
	
	// Information
	uint64_t	mac;
	uint64_t	input_bandwidth;
	uint64_t	output_bandwidth;
	uint16_t	padding_head;
	uint16_t	padding_tail;
	uint16_t	min_buffer_size;
	uint16_t	max_buffer_size;
	
	// Lock
	volatile uint8_t	input_lock;
	volatile uint8_t	output_lock;
	
	// Buffer
	FIFO*		input_buffer;
	FIFO*		output_buffer;
	
	// Statistics
	uint64_t	input_bps;
	uint64_t	input_dbps;
	uint64_t	output_bps;
	uint64_t	output_dbps;
	
	uint32_t	input_pps;
	uint32_t	input_dpps;
	uint32_t	output_pps;
	uint32_t	output_dpps;
	
	uint64_t	input_bytes;
	uint64_t	input_packets;
	uint64_t	input_drop_bytes;
	uint64_t	input_drop_packets;
	uint64_t	output_bytes;
	uint64_t	output_packets;
	uint64_t	output_drop_bytes;
	uint64_t	output_drop_packets;
	
	// Configuration
	Map*		config;
} NetworkInterface;

typedef Packet*(*NI_DPI)(Packet*);

/*
struct netif;
struct netif* ni_init(int idx, uint32_t ip, uint32_t netmask, uint32_t gw, bool is_default, 
		NI_DPI preprocessor, NI_DPI postprocessor);
bool ni_poll();
*/

int ni_count();
NetworkInterface* ni_get(int idx);

Packet* ni_alloc(NetworkInterface* ni, uint16_t size);
void ni_free(Packet* packet);

bool ni_has_input(NetworkInterface* ni);
Packet* ni_input(NetworkInterface* ni);
Packet* ni_tryinput(NetworkInterface* ni);
bool ni_output_available(NetworkInterface* ni);
bool ni_has_output(NetworkInterface* ni);
bool ni_output(NetworkInterface* ni, Packet* packet);
bool ni_output_dup(NetworkInterface* ni, Packet* packet);
bool ni_tryoutput(NetworkInterface* ni, Packet* packet);

size_t ni_pool_used(NetworkInterface* ni);
size_t ni_pool_free(NetworkInterface* ni);
size_t ni_pool_total(NetworkInterface* ni);

void ni_config_put(NetworkInterface* ni, char* key, void* data);
bool ni_config_contains(NetworkInterface* ni, char* key);
void* ni_config_remove(NetworkInterface* ni, char* key);
void* ni_config_get(NetworkInterface* ni, char* key);

#endif /* __NET_NETWORK_INTERFACE_H__ */
