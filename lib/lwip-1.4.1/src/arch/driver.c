/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"

#include <string.h>
#include <lwip/init.h>
#include <netif/etharp.h>
#include <net/nic.h>
#include <net/interface.h>
#include <util/list.h>
#include <lwip/timers.h>
#include <gmalloc.h>

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

struct netif_private {
	NIC*	nic;
	NIC_DPI	preprocessor;
	NIC_DPI	postprocessor;
};

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif)
{
  NIC* nic = ((struct netif_private*)netif->state)->nic;
  
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = (nic->mac >> 40) & 0xff;
  netif->hwaddr[1] = (nic->mac >> 32) & 0xff;
  netif->hwaddr[2] = (nic->mac >> 24) & 0xff;
  netif->hwaddr[3] = (nic->mac >> 16) & 0xff;
  netif->hwaddr[4] = (nic->mac >> 8) & 0xff;
  netif->hwaddr[5] = (nic->mac >> 0) & 0xff;

  /* maximum transfer unit */
  netif->mtu = 1500;
  
  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
 
  /* Do whatever else is needed to initialize interface. */  
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  NIC* nic = ((struct netif_private*)netif->state)->nic;
  struct pbuf *q;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  u16_t tot_len = p->tot_len;
  Packet* packet = nic_alloc(nic, tot_len);
  packet->end = packet->start + tot_len;
  int idx = 0;
  for(q = p; q != NULL; q = q->next) {
    memcpy(packet->buffer + packet->start + idx, q->payload, q->len);
    idx += q->len;
  }

  NIC_DPI postprocessor = ((struct netif_private*)netif->state)->postprocessor;
  if(postprocessor)
    packet = postprocessor(packet);
  
  if(packet)
    nic_output(nic, packet);

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
  
  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif, Packet* packet)
{
  struct pbuf *p, *q;
  u16_t len;

  len = packet->end - packet->start;

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  
  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    int idx = 0;
    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    for(q = p; q != NULL; q = q->next) {
      memcpy(q->payload, packet->buffer + packet->start + idx, q->len);
      idx += q->len;
    }
    nic_free(packet);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    nic_free(packet);
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
  }

  return p;  
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void
driver_poll(struct netif *netif, Packet* packet)
{
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif, packet);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = p->payload;

  switch (htons(ethhdr->type)) {
  /* IP or ARP packet? */
  case ETHTYPE_IP:
  case ETHTYPE_ARP:
#if PPPOE_SUPPORT
  /* PPPoE packet? */
  case ETHTYPE_PPPOEDISC:
  case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
    if (netif->input(p, netif)!=ERR_OK)
     { LWIP_DEBUGF(NETIF_DEBUG, ("driver_input: IP input error\n"));
       pbuf_free(p);
       p = NULL;
     }
    break;

  default:
    pbuf_free(p);
    p = NULL;
    break;
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
static err_t
driver_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));
    
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;
  
  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

static bool is_lwip_inited;
 //static struct netif* netifs[NIC_SIZE];
 //static struct netif_private* privates[NIC_SIZE];
 //static int netif_count;
static List* netifs = NULL;
static ListIterator iter;
//static List* privates;

struct netif* nic_init(NIC* nic, NIC_DPI preprocessor, NIC_DPI postprocessor) {
	if(!netifs) {
		netifs = list_create(NULL);
		if(!netifs) {
			printf("Cannot create netif list\n");
			return NULL;
		}
		list_iterator_init(&iter, netifs);
	}
 //	if(!privates) {
 //		privates = list_create(NULL);
 //		if(!privates) {
 //			printf("Cannot create netif list\n");
 //			return NULL;
 //		}
 //	}

	if(list_size(netifs) >= NIC_SIZE)
		return NULL;

	uint32_t ip;
	IPv4Interface* interface = NULL;

	Map* interfaces = nic_config_get(nic, NIC_ADDR_IPv4);
	if(!interfaces)
		return NULL;

	MapIterator iter;
	map_iterator_init(&iter, interfaces);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_next(&iter);
		interface = entry->data;
		ip = (uint32_t)(uint64_t)entry->key;
		break;
	}

	if(!interface)
		return NULL;

	uint32_t netmask = interface->netmask;
	uint32_t gw = interface->gateway;
	bool is_default = interface->_default;
	
	if(!is_lwip_inited) {
		lwip_init();
		is_lwip_inited = true;
	}
	
	struct netif* netif = gmalloc(sizeof(struct netif));
	//netifs[netif_count] = netif;
	struct netif_private* private =  gmalloc(sizeof(struct netif_private));
	list_add(netifs, netif);
	//list_add(privates, private);
	//privates[netif_count] = private;
	//netif_count++;

	private->nic = nic;
	private->preprocessor = preprocessor;
	private->postprocessor = postprocessor;
	
	struct ip_addr ip2, netmask2, gw2;
	IP4_ADDR(&ip2, (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, (ip >> 0) & 0xff);
	IP4_ADDR(&netmask2, (netmask >> 24) & 0xff, (netmask >> 16) & 0xff, (netmask >> 8) & 0xff, (netmask >> 0) & 0xff);
	IP4_ADDR(&gw2, (gw >> 24) & 0xff, (gw >> 16) & 0xff, (gw >> 8) & 0xff, (gw >> 0) & 0xff);
	
	netif_add(netif, &ip2, &netmask2, &gw2, private, driver_init, ethernet_input);
	if(is_default)
		netif_set_default(netif);
	netif_set_up(netif);
	
	return netif;
}

bool nic_remove(struct netif* netif) {
 	netif_set_down(netif);
 	netif_remove(netif);
	list_remove_data(netifs, netif);
	gfree(netif->state);
	gfree(netif);

	return true;
}

bool nic_poll() {
	bool result = false;
	
	if(list_size(netifs) == 0)
		return result;
	
	if(!list_iterator_has_next(&iter)) {
		list_iterator_init(&iter, netifs);
		goto check_has_next;
	} else
		goto process_packet;

check_has_next:
	if(list_iterator_has_next(&iter)) {
process_packet:
;
		struct netif* netif = list_iterator_next(&iter);
		NIC* nic = ((struct netif_private*)netif->state)->nic;
		if(!nic_has_input(nic))
			goto done;
		
		Packet* packet = nic_input(nic);
		NIC_DPI preprocessor = ((struct netif_private*)netif->state)->preprocessor;
		if(preprocessor)
			packet = preprocessor(packet);
		
		if(packet)
			driver_poll(netif, packet);
		
		result = true;
	} else {
		result = false;
	}

done:
	return result;
}

void nic_timer() {
	sys_check_timeouts();
}
