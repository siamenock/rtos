#include <gmalloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <net/ether.h>
#include "driver/nicdev.h"

NICDevice* nicdev_add_vlan(NICDevice* nicdev, uint16_t id) {
	if(((NICDriver*)nicdev->driver)->add_vid) {
		if(!((NICDriver*)nicdev->driver)->add_vid(nicdev, id))
			return NULL;
	}

	NICDevice* vlan_nicdev = gmalloc(sizeof(NICDevice));
	memset(vlan_nicdev, 0, sizeof(NICDevice));
	sprintf(vlan_nicdev->name, "%s.%d", nicdev->name, id);
	if(nicdev_get(vlan_nicdev->name)) {
		gfree(vlan_nicdev);
		return NULL;
	}
	vlan_nicdev->mac = nicdev->mac;
	vlan_nicdev->vlan_proto = ETHER_TYPE_8021Q;
	vlan_nicdev->vlan_tci = endian16(id);
	vlan_nicdev->driver = nicdev->driver;
	vlan_nicdev->priv = nicdev->priv;

	NICDevice* next = nicdev;
	while(1) {
		if(next->next) {
			if((endian16(next->next->vlan_tci) & 0xfff) < id) {
				next = next->next;
				continue;
			}

			next->next->prev = vlan_nicdev;
			vlan_nicdev->next = next->next;

			next->next = vlan_nicdev;
			vlan_nicdev->prev = next;
			break;
		} else {
			//Add last
			next->next = vlan_nicdev;
			vlan_nicdev->prev = next;
			vlan_nicdev->next = NULL;
			break;
		}
	}

	event_busy_add(((NICDriver*)vlan_nicdev->driver)->xmit, vlan_nicdev);
	return vlan_nicdev;
}

bool nicdev_remove_vlan(NICDevice* nicdev) {
	// Check has nicdevice
	// for(int i = 0; i < MAX_NIC_DEVICE_COUNT; i++) {
	// 	NICDevice* dev = nic_devices[i];
	// 	if(dev)
	// 		return false;
	// 	else
	// 		break;
	// }

	// if(nicdev->vlan_proto != ETHER_TYPE_8021Q)
	// 		return false;

	// if(((NICDriver*)nicdev->driver)->remove_vid) {
	// 	((NICDriver*)nicdev->driver)->remove_vid(nicdev, endian16(nicdev->vlan_tci) & 0xfff);
	// }

	// gfree(nicdev);
	//FIXME remove event nicdev
	return false;
}