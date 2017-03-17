#ifndef _VIRTIO_NET_H_
#define _VIRTIO_NET_H_

/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers. */
/* The feature bitmap for virtio net */
#define VIRTIO_NET_F_CSUM		0	/* Host handles pkts w/ partial csum */
#define VIRTIO_NET_F_GUEST_CSUM		1	/* Guest handles pkts w/ partial csum */
#define VIRTIO_NET_F_MAC		5	/* Host has given MAC address. */
#define VIRTIO_NET_F_GSO		6	/* Host handles pkts w/ any GSO type */
#define VIRTIO_NET_F_GUEST_TSO4		7	/* Guest can handle TSOv4 in. */
#define VIRTIO_NET_F_GUEST_TSO6		8	/* Guest can handle TSOv6 in. */
#define VIRTIO_NET_F_GUEST_ECN		9	/* Guest can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_GUEST_UFO		10	/* Guest can handle UFO in. */
#define VIRTIO_NET_F_HOST_TSO4		11	/* Host can handle TSOv4 in. */
#define VIRTIO_NET_F_HOST_TSO6		12	/* Host can handle TSOv6 in. */
#define VIRTIO_NET_F_HOST_ECN		13	/* Host can handle TSO[6] w/ ECN in. */
#define VIRTIO_NET_F_HOST_UFO		14	/* Host can handle UFO in. */
#define VIRTIO_NET_F_MRG_RXBUF		15	/* Host can merge receive buffers. */
#define VIRTIO_NET_F_STATUS		16	/* virtio_net_config.status available */
#define VIRTIO_NET_F_CTRL_VQ		17	/* Control channel available */
#define VIRTIO_NET_F_CTRL_RX		18	/* Control channel RX mode support */
#define VIRTIO_NET_F_CTRL_VLAN		19	/* Control channel VLAN filtering */
#define VIRTIO_NET_F_CTRL_RX_EXTRA 	20	/* Extra RX mode control support */
#define VIRTIO_NET_F_GUEST_ANNOUNCE 	21	/* Guest can announce device on the
						 * network */
#define VIRTIO_NET_F_MQ			22	/* Device supports Receive Flow
						 * Steering */
#define VIRTIO_NET_F_CTRL_MAC_ADDR 	23	/* Set MAC address */

#define VIRTIO_NET_S_LINK_UP		1	/* Link is up */
#define VIRTIO_NET_S_ANNOUNCE		2	/* Announcement is needed */

/* Provided device drvier features on PacketNgin */
const uint32_t feature_table[] = {
	VIRTIO_NET_F_MAC,
	VIRTIO_NET_F_MRG_RXBUF, 
	VIRTIO_NET_F_CTRL_VQ,
};

typedef struct {
#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1	// Use csum_start, csum_offset
	uint8_t flags;
#define VIRTIO_NET_HDR_GSO_NONE		0	// Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4	1	// GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP		3	// GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6	4	// GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN		0x80	// TCP has ECN set
	uint8_t gso_type;
	uint16_t hdr_len;		/* Ethernet + IP + tcp/udp hdrs */
	uint16_t gso_size;		/* Bytes to append to hdr_len per frame */
	uint16_t csum_start;		/* Position to start checksumming from */
	uint16_t csum_offset;		/* Offset after that to place checksum */
	uint16_t num_buffers;
} __attribute__((packed)) VirtIONetHDR;

typedef struct {
	/* The config defining mac address (if VIRTIO_NET_F_MAC) */
	uint8_t mac[6];
	/* See VIRTIO_NET_F_STATUS and VIRTIO_NET_S_* above */
	uint16_t status;
} __attribute__((packed)) VirtIONetConfig;

/* Packet Structure that PacketNgin uses */
typedef struct {
	VirtIONetHDR vhdr;
	uint8_t data[0];
} __attribute__((packed)) VirtIONetPacket;

/*
 * Control virtqueue data structures
 *
 * The control virtqueue expects a header in the first sg entry
 * and an ack/status response in the last entry.  Data for the
 * command goes in between.
 */
typedef struct {
	uint8_t class;
	uint8_t cmd;
	uint8_t cmd_specific_data;
	uint8_t ack;
} __attribute__((packed)) VirtIONetCtrlPacket;

typedef uint8_t virtio_net_ctrl_ack;

#define VIRTIO_NET_OK     0
#define VIRTIO_NET_ERR    1

/*
 * Control the RX mode, ie. promisucous, allmulti, etc...
 * All commands require an "out" sg entry containing a 1 byte
 * state value, zero = disable, non-zero = enable.  Commands
 * 0 and 1 are supported with the VIRTIO_NET_F_CTRL_RX feature.
 * Commands 2-5 are added with VIRTIO_NET_F_CTRL_RX_EXTRA.
 */
#define VIRTIO_NET_CTRL_RX    		0
#define VIRTIO_NET_CTRL_RX_PROMISC      0
#define VIRTIO_NET_CTRL_RX_ALLMULTI     1
#define VIRTIO_NET_CTRL_RX_ALLUNI       2
#define VIRTIO_NET_CTRL_RX_NOMULTI      3
#define VIRTIO_NET_CTRL_RX_NOUNI        4
#define VIRTIO_NET_CTRL_RX_NOBCAST      5

/*
 * Control the MAC filter table.
 *
 * The MAC filter table is managed by the hypervisor, the guest should
 * assume the size is infinite.  Filtering should be considered
 * non-perfect, ie. based on hypervisor resources, the guest may
 * received packets from sources not specified in the filter list.
 *
 * In addition to the class/cmd header, the TABLE_SET command requires
 * two out scatterlists.  Each contains a 4 byte count of entries followed
 * by a concatenated byte stream of the ETH_ALEN MAC addresses.  The
 * first sg list contains unicast addresses, the second is for multicast.
 * This functionality is present if the VIRTIO_NET_F_CTRL_RX feature
 * is available.
 */

#define ETH_ALEN 	6

typedef struct {
	uint32_t entries;
	uint8_t macs[][ETH_ALEN];
} __attribute__((packed)) VirtIONetCtrlMac;

#define VIRTIO_NET_CTRL_MAC    		1
#define VIRTIO_NET_CTRL_MAC_TABLE_SET	0

/*
 * Control VLAN filtering
 *
 * The VLAN filter table is controlled via a simple ADD/DEL interface.
 * VLAN IDs not added may be filterd by the hypervisor.  Del is the
 * opposite of add.  Both commands expect an out entry containing a 2
 * byte VLAN ID.  VLAN filterting is available with the
 * VIRTIO_NET_F_CTRL_VLAN feature bit.
 */
#define VIRTIO_NET_CTRL_VLAN       	2
#define VIRTIO_NET_CTRL_VLAN_ADD        0
#define VIRTIO_NET_CTRL_VLAN_DEL        1

#endif /* _VIRTIO_NET_H_ */
