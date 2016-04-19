#ifndef __NET_NIC_H__
#define __NET_NIC_H__

#include <stdint.h>
#include <stdbool.h>
#include <net/interface.h>
#include <net/packet.h>
#include <util/fifo.h>
#include <util/map.h>

/**
 * @file
 * Network Interface Controller (NIC) management
 */

/**
 * NIC attributes to create or update NIC.
 */
typedef enum {
	NIC_NONE,		///< End of attributes
	NIC_MAC,		///< MAC address
	//NIC_PORT,		///< Physical port mapping
	NIC_DEV,		///< Device of Network Interface
	NIC_POOL_SIZE,		///< NI's total memory size
	NIC_INPUT_BANDWIDTH,	///< Input bandwidth in bps
	NIC_OUTPUT_BANDWIDTH,	///< Output bandwidth in bps
	NIC_PADDING_HEAD,	///< Minimum padding head of packet payload buffer
	NIC_PADDING_TAIL,	///< Minimum padding tail of packet payload buffer
	NIC_INPUT_BUFFER_SIZE,	///< Number of input buffer size, the packet count
	NIC_OUTPUT_BUFFER_SIZE,	///< Number of output buffer size, the packet count
	NIC_MIN_BUFFER_SIZE,	///< Minimum packet payload buffer size which includes padding
	NIC_MAX_BUFFER_SIZE,	///< Maximum packet payload buffer size which includes padding
	NIC_INPUT_ACCEPT_ALL,	///< To accept all packets to receive
	NIC_INPUT_ACCEPT,	///< List of accept MAC addresses to receive
	NIC_OUTPUT_ACCEPT_ALL,	///< To accept all packets to send
	NIC_OUTPUT_ACCEPT,	///< List of accept MAC addresses to send
	NIC_INPUT_FUNC,		///< Deprecated
} NIC_ATTRIBUTES;

#define NIC_SIZE			256		///< VM's maximum number of NICs

#define PACKET_STATUS_IP		(1 >> 0)	///< Is IP(v4, v6) packet? (deprecated)
#define PACKET_STATUS_IPv6		(1 >> 1)	///< Is IPv6 packet? (deprectated)
#define PACKET_STATUS_UDP		(1 >> 2)	///< Is UDP packet? (deprectated)
#define PACKET_STATUS_TCP		(1 >> 3)	///< Is TCP packet? (deprectated)
#define PACKET_STATUS_L2_CHECKSUM	(1 >> 4)	///< Is level 2(IP, IPv6) RX checksum OK or TX checksum already calculated (deprectated)
#define PACKET_STATUS_L3_CHECKSUM	(1 >> 5)	///< Is level 3(TCP, UDP) RX checksum OK or TX checksum already calculated (deprectated)

/** 
 * Network interface or just NIC.
 */
typedef struct _NIC {
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
} NIC;

/**
 * Pre/post processor of LwIP network protocol stack
 */
typedef Packet*(*NIC_DPI)(Packet*);

/**
 * Initialize LwIP network protocol stack if needed
 */
struct netif;
struct netif* nic_init(NIC* nic, NIC_DPI preprocessor, NIC_DPI postprocessor);

/**
 * Remove LwIP network protocol stack if needed
 */
bool nic_remove(struct netif* netif);

/**
 * Poll NIC to receive and send packets.
 *
 * @return Packet is processed
 */
bool nic_poll();

/**
 * Process time based events of LwIP network protocol stack. It must be called every 100ms,
 * when using LwIP
 */
void nic_timer();

/**
 * Get number of NICs of VM.
 *
 * @return number of NICs of VM
 */
int nic_count();

/**
 * Get idx'th NIC of VM.
 *
 * @param idx index number of NICs which is zero based.
 * @return reference of NIC.
 */
NIC* nic_get(int idx);

/**
 * Allocate a Packet from NIC.
 *
 * @param nic NIC reference
 * @param size packet payload buffer size without padding
 * @return allocated Packet or NULL if there is no memory in NIC
 */
Packet* nic_alloc(NIC* nic, uint16_t size);

/**
 * Drop and free packet explicitly. nic_output frees packet automatically, 
 * so it must not be freed again when nic_output is succeed.
 *
 * @param packet packet reference
 */
void nic_free(Packet* packet);

/**
 * Check there is any packet in NIC's input buffer.
 *
 * @param nic NIC reference
 * @return true if there is one or more packets in NIC's input buffer
 */
bool nic_has_input(NIC* nic);

/**
 * Receive a packet from NIC's input buffer. This functions waits when input buffer is locked.
 *
 * @param nic NIC reference
 * @return packet reference or NULL if there is no available packet
 */
Packet* nic_input(NIC* nic);

/**
 * Receive a packet from NIC's input buffer. This functions DOES NOT waits when input buffer is locked.
 *
 * @param nic NIC reference
 * @return packet reference or NULL if there is no available packet or input buffer is locked
 */
Packet* nic_tryinput(NIC* nic);

/**
 * Check NIC's output buffer is available.
 *
 * @param nic NIC reference
 * @return true if there is available buffer
 */
bool nic_output_available(NIC* nic);

/**
 * Check there is any packet in NIC's output buffer.
 *
 * @param nic NIC reference
 * @return true if there is one or more packets in NIC's output buffer
 */
bool nic_has_output(NIC* nic);

/**
 * Send a packet to NIC's output buffer. This functions waits when output buffer is locked.
 * When it returns false, the packet must be retried to send or explicitly dropped.
 *
 * @param nic NIC reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full 
 * packet must be explicitly treated(drop or resend)
 */
bool nic_output(NIC* nic, Packet* packet);

/**
 * Send a packet to NIC's output buffer. This functions waits when output buffer is locked.
 * The packet must be explicitly treated (drop or send) regardless the return value.
 *
 * @param nic NIC reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full 
 */
bool nic_output_dup(NIC* nic, Packet* packet);

/**
 * Send a packet to NIC's output buffer. This functions DOES NOT waits when output buffer is locked.
 * When it returns false, the packet must be retried to send or explicitly dropped.
 *
 * @param nic NIC reference
 * @param packet packet reference to send
 * @return true if output is succeed or false if output buffer is full or locked
 * packet must be explicitly treated
 */
bool nic_tryoutput(NIC* nic, Packet* packet);

/**
 * Get the used memory size of NIC
 *
 * @return size of used memory of NIC in bytes
 */
size_t nic_pool_used(NIC* nic);

/**
 * Get the free memory size of NIC
 *
 * @return size of free memory of NIC in bytes
 */
size_t nic_pool_free(NIC* nic);

/**
 * Get the total memory size of NIC
 *
 * @return size of total memory of NIC in bytes
 */
size_t nic_pool_total(NIC* nic);

/**
 * Put NIC's configuration data. It's safe to put data in multiple times with same key.
 *
 * @param nic NIC reference
 * @param key configuration key
 * @param data configuration data
 */
bool nic_config_put(NIC* nic, char* key, void* data);

/**
 * Check configuration data is setted with the key.
 *
 * @param nic NIC reference
 * @param key configuration key
 * @return true if configuration data is setted
 */
bool nic_config_contains(NIC* nic, char* key);

/**
 * Remove NIC's configuration data with the key.
 *
 * @param nic NIC reference
 * @param key configuration key
 * @return configuration data reviously putted or NULL if not putted
 */
void* nic_config_remove(NIC* nic, char* key);

/**
 * Get NIC's configuration data with the key.
 *
 * @param nic NIC reference
 * @param key configuration key
 * @return configuration data reviously putted or NULL if not putted
 */
void* nic_config_get(NIC* nic, char* key);

/**
 * Add IP with the interface.
 *
 * @param nic NIC reference
 * @param addr
 * @return true if success
 */
bool nic_ip_add(NIC* nic, uint32_t addr);

/**
 * Get ipv4 interface.
 *
 * @param nic NIC reference
 * @param addr
 * @return interface of ipv4
 */
IPv4Interface* nic_ip_get(NIC* nic, uint32_t addr);

/**
 * Remove IP with the interface.
 *
 * @param nic NIC reference
 * @param addr
 * @return interface of ipv4
 */
bool nic_ip_remove(NIC* nic, uint32_t addr);

#endif /* __NET_NIC_H__ */
