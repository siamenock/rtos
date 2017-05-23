#ifndef __LWIP_DRIVER__
#define __LWIP_DRIVER__

typedef Packet* (*NIC_DPI)(Packet*);

struct netif_private {
	NIC*	nic;
	NIC_DPI	rx_process;
	NIC_DPI	tx_process;
};

err_t lwip_driver_init(struct netif *netif);
bool lwip_nic_remove(struct netif* netif);
bool lwip_nic_poll(struct netif* netif, Packet* packet);
void lwip_nic_timer();
#endif /*__LWIP_DRIVER__*/
