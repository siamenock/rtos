#ifndef __NET_VLAN_H__
#define __NET_VLAN_H__

#include <net/packet.h>

/**
 * @file
 * IEEE 802.1Q
 */

#define VLAN_LEN			2	///< VLANv4 header length

/**
 * Priority levels
*/
#define	VLAN_BACKGROUND			0	//lowest
#define VLAN_BEST_EFFORT 		1
#define VLAN_EXCELLENT_EFFORT		2
#define VLAN_CRITICAL_APPLICATIONS	3
#define VLAN_VIDEO			4
#define VLAN_VOICE			5
#define VLAN_INTERNETWORK_CONTROL	6
#define VLAN_NETWORK_CONTROL		7	//highest

#define VLAN_GET_PCP(tci)			(tci & 0xe000) >> 13
#define VLAN_GET_DEI(tci)			(tci & 0x1000) >> 12
#define VLAN_GET_VID(tci)			(tci & 0xfff)

/**
 * VLANv4 header
 */
typedef struct _VLAN {
	uint16_t	tci;		/// pcd(3) + dei(1) + vid(12)
	uint16_t	type;
	//uint16_t	pcd: 3;		///< Priority code point
	//uint16_t	dei: 1;		///< Drop eligible indicator
	//uint16_t	vid: 12;	///< vLAN identifier 0x000 ~ 0xfff
	
	uint8_t		body[0];	///< VLAN body payload
} __attribute__ ((packed)) VLAN;

#endif /* __NET_VLAN_H__ */
