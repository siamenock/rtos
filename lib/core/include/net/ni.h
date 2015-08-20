#ifndef __NET_NETWORK_INTERFACE_H__
#define __NET_NETWORK_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <net/interface.h>
#include <net/packet.h>
#include <util/fifo.h>
#include <util/map.h>

/**
 * @file
 * Network Interface (NI) management
 */

/**
 * NI attributes to create or update NI.
 */
typedef enum {
	NI_NONE,		///< End of attributes
	NI_MAC,			///< MAC address
	//NI_PORT,		///< Physical port mapping
	NI_DEV,			///< Device of Network Interface
	NI_POOL_SIZE,		///< NI's total memory size
	NI_INPUT_BANDWIDTH,	///< Input bandwidth in bps
	NI_OUTPUT_BANDWIDTH,	///< Output bandwidth in bps
	NI_PADDING_HEAD,	///< Minimum padding head of packet payload buffer
	NI_PADDING_TAIL,	///< Minimum padding tail of packet payload buffer
	NI_INPUT_BUFFER_SIZE,	///< Number of input buffer size, the packet count
	NI_OUTPUT_BUFFER_SIZE,	///< Number of output buffer size, the packet count
	NI_MIN_BUFFER_SIZE,	///< Minimum packet payload buffer size which includes padding
	NI_MAX_BUFFER_SIZE,	///< Maximum packet payload buffer size which includes padding
	NI_INPUT_ACCEPT_ALL,	///< To accept all packets to receive
	NI_INPUT_ACCEPT,	///< List of accept MAC addresses to receive
	NI_OUTPUT_ACCEPT_ALL,	///< To accept all packets to send
	NI_OUTPUT_ACCEPT,	///< List of accept MAC addresses to send
	NI_INPUT_FUNC,		///< Deprecated
} NI_ATTRIBUTES;

#define NIS_SIZE			256		///< VM's maximum number of NIs

#define PACKET_STATUS_IP		(1 >> 0)	///< Is IP(v4, v6) packet? (deprecated)
#define PACKET_STATUS_IPv6		(1 >> 1)	///< Is IPv6 packet? (deprectated)
#define PACKET_STATUS_UDP		(1 >> 2)	///< Is UDP packet? (deprectated)
#define PACKET_STATUS_TCP		(1 >> 3)	///< Is TCP packet? (deprectated)
#define PACKET_STATUS_L2_CHECKSUM	(1 >> 4)	///< Is level 2(IP, IPv6) RX checksum OK or TX checksum already calculated (deprectated)
#define PACKET_STATUS_L3_CHECKSUM	(1 >> 5)	///< Is level 3(TCP, UDP) RX checksum OK or TX checksum already calculated (deprectated)

/** 
 * Network interface or just NI.
 */
typedef struct _NetworkInterface {
	// Management
	uint64_t	pool_size;		///< Memory pool size (read only)
	void*		pool;			///< Base pool address (read only)
	
	// Information
	uint64_t	mac;			///< MAC address (read only)
	uint64_t	input_bandwidth;	///< Input bandwidth in bps (read only)
	uint64_t	output_bandwidth;	///< Output bandwidth in bps (read only)
	uint16_t	padding_head;		///< Payload buffer head padding in bytes (read only)
	uint16_t	padding_tail;		///< Payload buffer tail padding in bytes (read only)
	uint16_t	min_buffer_size;	///< Minimum payload buffer size include padding (read only)
	uint16_t	max_buffer_size;	///< Maximum payload buffer size include padding (read only)
	
	// Lock
	volatile uint8_t	input_lock;	///< Input buffer lock (DO NOT USE IT DIRECTLY)
	volatile uint8_t	output_lock;	///< Output buffer lock (DO NOT USE IT DIRECTLY)
	
	// Buffer
	FIFO*		input_buffer;		///< Input buffer (DO NOT USE IT DIRECTLY)
	FIFO*		output_buffer;		///< Output buffer (DO NOT USE IT DIRECTLY)
	
	// Statistics
	uint64_t	input_bps;		///< Input bits per a second (read only)
	uint64_t	input_dbps;		///< Input dropped bits per a second (read only)
	uint64_t	output_bps;		///< Output bits per a second (read only)
	uint64_t	output_dbps;		///< Output dropped bits per a second (read only)
	
	uint32_t	input_pps;		///< Input packets per a second (read only)
	uint32_t	input_dpps;		///< Input dropped packets per a second (read only)
	uint32_t	output_pps;		///< Output packets per a second (read only)
	uint32_t	output_dpps;		///< Output dropped packets per a second (read only)
	
	uint64_t	input_bytes;		///< Total input bytes (read only)
	uint64_t	input_packets;		///< Total input packets (read only)
	uint64_t	input_drop_bytes;	///< Total dropped input bytes (read only)
	uint64_t	input_drop_packets;	///< Total dropped input packets (read only)
	uint64_t	output_bytes;		///< Total output bytes (read only)
	uint64_t	output_packets;		///< Total output packets (read only)
	uint64_t	output_drop_bytes;	///< Total dropped output bytes (read only)
	uint64_t	output_drop_packets;	///< Total dropped output packets (read only)
	
	// Configuration
	Map*		config;			///< Configuration collection (DO NOT USE IT DIRECTLY)
} NetworkInterface;

/**
 * Pre/post processor of LwIP network protocol stack
 */
typedef Packet*(*NI_DPI)(Packet*);

/**
 * Initialize LwIP network protocol stack if needed
 */
struct netif;
struct netif* ni_init(NetworkInterface* ni, NI_DPI preprocessor, NI_DPI postprocessor);

/**
 * Remove LwIP network protocol stack if needed
 */
bool ni_remove(struct netif* netif);

/**
 * Poll NI to receive and send packets.
 *
 * @return Packet is processed
 */
bool ni_poll();

/**
 * Process time based events of LwIP network protocol stack. It must be called every 100ms,
 * when using LwIP
 */
void ni_timer();

/**
 * Get number of NIs of VM.
 *
 * @return number of NIs of VM
 */
int ni_count();

/**
 * Get idx'th NI of VM.
 *
 * @param idx index number of NIs which is zero based.
 * @return reference of NI.
 */
NetworkInterface* ni_get(int idx);

/**
 * Allocate a Packet from NI.
 *
 * @param ni NI reference
 * @param size packet payload buffer size without padding
 * @return allocated Packet or NULL if there is no memory in NI
 */
Packet* ni_alloc(NetworkInterface* ni, uint16_t size);

/**
 * Drop and free packet explicitly. ni_output frees packet automatically, 
 * so it must not be freed again when ni_output is succeed.
 *
 * @param packet packet reference
 */
void ni_free(Packet* packet);

/**
 * Check there is any packet in NI's input buffer.
 *
 * @param ni NI reference
 * @return true if there is one or more packets in NI's input buffer
 */
bool ni_has_input(NetworkInterface* ni);

/**
 * Receive a packet from NI's input buffer. This functions waits when input buffer is locked.
 *
 * @param ni NI reference
 * @return packet reference or NULL if there is no available packet
 */
Packet* ni_input(NetworkInterface* ni);

/**
 * Receive a packet from NI's input buffer. This functions DOES NOT waits when input buffer is locked.
 *
 * @param ni NI reference
 * @return packet reference or NULL if there is no available packet or input buffer is locked
 */
Packet* ni_tryinput(NetworkInterface* ni);

/**
 * Check NI's output buffer is available.
 *
 * @param ni NI reference
 * @return true if there is available buffer
 */
bool ni_output_available(NetworkInterface* ni);

/**
 * Check there is any packet in NI's output buffer.
 *
 * @param ni NI reference
 * @return true if there is one or more packets in NI's output buffer
 */
bool ni_has_output(NetworkInterface* ni);

/**
 * Send a packet to NI's output buffer. This functions waits when output buffer is locked.
 * When it returns false, the packet must be retried to send or explicitly dropped.
 *
 * @param ni NI reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full 
 * packet must be explicitly treated(drop or resend)
 */
bool ni_output(NetworkInterface* ni, Packet* packet);

/**
 * Send a packet to NI's output buffer. This functions waits when output buffer is locked.
 * The packet must be explicitly treated (drop or send) regardless the return value.
 *
 * @param ni NI reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full 
 */
bool ni_output_dup(NetworkInterface* ni, Packet* packet);

/**
 * Send a packet to NI's output buffer. This functions DOES NOT waits when output buffer is locked.
 * When it returns false, the packet must be retried to send or explicitly dropped.
 *
 * @param ni NI reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full or locked
 * packet must be explicitly treated
 */
bool ni_tryoutput(NetworkInterface* ni, Packet* packet);

/**
 * Get the used memory size of NI
 *
 * @return size of used memory of NI in bytes
 */
size_t ni_pool_used(NetworkInterface* ni);

/**
 * Get the free memory size of NI
 *
 * @return size of free memory of NI in bytes
 */
size_t ni_pool_free(NetworkInterface* ni);

/**
 * Get the total memory size of NI
 *
 * @return size of total memory of NI in bytes
 */
size_t ni_pool_total(NetworkInterface* ni);

/**
 * Put NI's configuration data. It's safe to put data in multiple times with same key.
 *
 * @param ni NI reference
 * @param key configuration key
 * @param data configuration data
 */
bool ni_config_put(NetworkInterface* ni, char* key, void* data);

/**
 * Check configuration data is setted with the key.
 *
 * @param ni NI reference
 * @param key configuration key
 * @return true if configuration data is setted
 */
bool ni_config_contains(NetworkInterface* ni, char* key);

/**
 * Remove NI's configuration data with the key.
 *
 * @param ni NI reference
 * @param key configuration key
 * @return configuration data reviously putted or NULL if not putted
 */
void* ni_config_remove(NetworkInterface* ni, char* key);

/**
 * Get NI's configuration data with the key.
 *
 * @param ni NI reference
 * @param key configuration key
 * @return configuration data reviously putted or NULL if not putted
 */
void* ni_config_get(NetworkInterface* ni, char* key);

/**
 * Add IP with the interface.
 *
 * @param ni NI reference
 * @param addr
 * @return true if success
 */
bool ni_ip_add(NetworkInterface* ni, uint32_t addr);

/**
 * Get ipv4 interface.
 *
 * @param ni NI reference
 * @param addr
 * @return interface of ipv4
 */
IPv4Interface* ni_ip_get(NetworkInterface* ni, uint32_t addr);

/**
 * Remove IP with the interface.
 *
 * @param ni NI reference
 * @param addr
 * @return interface of ipv4
 */
bool ni_ip_remove(NetworkInterface* ni, uint32_t addr);

#endif /* __NET_NETWORK_INTERFACE_H__ */
