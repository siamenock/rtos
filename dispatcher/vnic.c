#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/types.h>

#include "packet.h"
#include "map.h"
#include "ether.h"
#include "vlan.h"
#include "ip.h"
#include "nic.h"
#include "vnic.h"

#define DEBUG		0
#define _ALIGN		16

#define printf printk

extern Map* nics;

uint64_t nic_rx;
uint64_t nic_tx;
uint64_t nic_tx2;

static bool vlan_input_process(VNIC* vnic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	uint16_t type = endian16(ether->type);
	if(type == ETHER_TYPE_8021Q) {
		VLAN* vlan = (VLAN*)ether->payload;
		uint16_t vid = VLAN_GET_VID(endian16(vlan->tci));
		if((vnic->port & 0xfff) != vid) {
			return false;
		}

		memmove((uint8_t*)ether + 4 , ether, ETHER_LEN - 2);
		packet->start += 4;

		return true;
	}

	return false;
}

static bool vlan_output_process(VNIC* vnic, Packet* packet) {
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	// Check start point
	if(packet->start < 4) {
		return false;
	}

	memmove((uint8_t*)ether - 4, ether, ETHER_LEN - 2);
	packet->start -= 4;
	ether = (void*)(packet->buffer + packet->start);
	ether->type = endian16(ETHER_TYPE_8021Q);
	VLAN* vlan = (VLAN*)ether->payload;
	vlan->tci = endian16(vnic->port & 0xfff);

	return true;
}

//extern int MANAGER_PID;

/*static bool done = false;*/
/*static void set_pid() {*/
	/*printf("Set pid \n");*/
	/*struct task_struct* task = pid_task(find_vpid(MANAGER_PID), PIDTYPE_PID);*/
	/*if(task) {*/
		/*printk("PID : %d, Name : %s\n", current->pid, current->comm);*/
		/*printk("Active %p\n", current->active_mm);*/
		/*printk("Owner %s\n", current->active_mm->owner->comm);*/
		/*printk("MM %p\n", current->mm);*/

		/*printk("Manager-------\n");*/
		/*printk("PID : %d, Name : %s\n", task->pid, task->comm);*/
		/*printk("Active %p\n", task->active_mm);*/
		/*printk("Owner %s\n", task->active_mm->owner->comm);*/
		/*printk("MM %p\n", task->mm);*/

	/*//	current->active_mm = task->mm;*/
	/*}*/

	/*done = true;*/
/*}*/
/*bool is_manager_context() {*/
	/*
	 *if(current->mm) {
	 *        printf("Current MM is : %s\n", current->mm->owner->comm);
	 *        printf("Current PID is : %d\n", current->pid);
	 *} else {
	 *        printf("MM is NULL!\n");
	 *}
	 */

	/*if(!done)*/
		/*set_pid();*/

	/*if(!current) {*/
		/*printf("Current is NULL\n");*/
		/*return false;*/
	/*}*/

	/*if(!current->active_mm) {*/
		/*printf("Active MM is NULL! \n");*/
		/*return false;*/
	/*}*/

	/*if(!current->active_mm->owner) {*/
		/*if(printk_ratelimit())*/
			/*printf("Owner is NULL\n");*/
		/*return false;*/
	/*} else {*/
/*//		printf("Owner pid is %p\n", current->active_mm->owner->pid);*/
	/*}*/
	
	/*if(current->active_mm->owner->pid != MANAGER_PID) {*/
		/*if(printk_ratelimit()) */
			/*printf("Not manager %s %d/%d\n", current->active_mm->owner->comm,*/
					/*not_manager_count, manager_count);*/
		/*not_manager_count++;*/
		/*return false;*/
	/*}*/
	/*manager_count++;*/
	/*return true;*/
/*}*/

bool nic_process_input(uint8_t local_port, uint8_t* buf1, uint32_t size1, uint8_t* buf2, uint32_t size2) {
	// Get dmac, smac
	uint64_t dmac;
	uint64_t smac;
	uint16_t vid = 0;

	if(size1 >= 14) {
		Ether* ether = (Ether*)buf1;
		dmac = endian48(ether->dmac);
		smac = endian48(ether->smac);
		if(endian16(ether->type) == ETHER_TYPE_8021Q) {
			if(size1 >= 16) {
				VLAN* vlan = (VLAN*)ether->payload;
				vid = VLAN_GET_VID(endian16(vlan->tci));
			} else {
				VLAN vlan;
				uint8_t* p = (uint8_t*)&vlan;
				ssize_t _size = size1 - 14;
				memcpy(p, buf1, _size);
				memcpy(p + _size, buf2, 2 - _size);
				vid = VLAN_GET_VID(endian16(vlan.tci));
			}
		}
	} else {
		Ether ether;
		uint8_t* p = (uint8_t*)&ether;
		memcpy(p, buf1, size1);

		ssize_t _size = 14 - size1;
		memcpy(p + size1, buf2, _size);
		dmac = endian48(ether.dmac);
		smac = endian48(ether.smac);
		if(endian16(ether.type) == ETHER_TYPE_8021Q) {
			VLAN vlan;
			p = (uint8_t*)&vlan;
			memcpy(p, buf2 + _size, 2);
			vid = VLAN_GET_VID(endian16(vlan.tci));
		}
	}


	// TODO: timer setting
	uint64_t time = (uint64_t)-1; //timer_frequency();

#if DEBUG
	printf("Input:  [%02d] %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
			local_port,
			(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
			(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
			(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
			(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
#endif /* DEBUG */

	bool input(VNIC* vnic) {
		if(vnic->input_accept && list_index_of(vnic->input_accept, (void*)smac, NULL) < 0)
			return false;

		uint16_t size = size1 + size2;

		if(vnic->input_closed - vnic->input_wait_grace > time) {
			printf("closed dropped %016lx ", vnic->mac);
			goto dropped;
		}

		uint16_t buffer_size = vnic->padding_head + size + vnic->padding_tail + (_ALIGN - 1);
		if(buffer_size > vnic->max_buffer_size) {
			printf("buffer dropped %016lx ", vnic->mac);
			goto dropped;
		}

		if(buffer_size < vnic->min_buffer_size)
			buffer_size = vnic->min_buffer_size;

		if(!fifo_available(vnic->nic->input_buffer)) {
#if DEBUG
			printf("fifo dropped %016lx ", vnic->mac);
#endif
			goto dropped;
		}

		Packet* packet = nic_alloc(vnic->nic, buffer_size);
		if(!packet) {
			printf("memory dropped %016lx ", vnic->mac);
			goto dropped;
		}

		// Copy
		if(size1 > 0)
			memcpy(packet->buffer + packet->start, buf1, size1);

		if(size2 > 0)
			memcpy(packet->buffer + packet->start + size1, buf2, size2);


		if(vnic->port & 0xfff) {
			if(!vlan_input_process(vnic, packet)) {
				nic_free(packet);
				goto dropped;
			}
		}

		// Push
		if(!fifo_push(vnic->nic->input_buffer, packet)) {
			nic_free(packet);
#if DEBUG
			printf("fifo dropped %016lx ", vnic->mac);
#endif
			goto dropped;
		}

		vnic->nic->input_bytes += size;
		vnic->nic->input_packets++;
		if(vnic->input_closed > time)
			vnic->input_closed += vnic->input_wait * size;
		else
			vnic->input_closed = time + vnic->input_wait * size;

		return true;

dropped:
		vnic->nic->input_drop_bytes += size;
		vnic->nic->input_drop_packets++;

		return false;
	}

	/*
	 *Map* vnics = map_get(((NICPriv*)nic_current->priv)->nics, (void*)(uint64_t)((local_port << 12) | vid));
	 */
	Map* vnics = map_get(nics, (void*)(uint64_t)((local_port << 12) | vid));
	if(!vnics) {
		printf("Could not find VNICs\n");
		return false;
	}

	if(dmac & ETHER_MULTICAST) {
		MapIterator iter;
		map_iterator_init(&iter, vnics);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			VNIC* vnic = entry->data;

			if(input(vnic)) {
				nic_rx++;
			}
		}
	} else {
		VNIC* vnic = map_get(vnics, (void*)dmac);
		if(!vnic)
			vnic = map_get(vnics, (void*)(uint64_t)0xffffffffffff);

		if(!vnic) {
			return false;
		}

		if(input(vnic)) {
			nic_rx++;
			return true;
		}
	}

	return false;
}

Packet* nic_process_output(uint8_t local_port) {
	uint64_t time = -1; //timer_frequency();

	// Map* nics = nics; //((NICPriv*)nic_current->priv)->nics;

	MapIterator iter0;
	map_iterator_init(&iter0, nics);
	while(map_iterator_has_next(&iter0)) {
		MapEntry* entry0 = map_iterator_next(&iter0);
		uint16_t port = (uint16_t)(uint64_t)entry0->key;
		if(local_port != (port >> 12)) {
			continue;
		}

		Map* vnics = entry0->data;
		if(!vnics)
			break;

		MapIterator iter1;
		map_iterator_init(&iter1, vnics);

		while(map_iterator_has_next(&iter1)) {
			MapEntry* entry1 = map_iterator_next(&iter1);
			VNIC* vnic = entry1->data;

			if(vnic->output_closed - vnic->output_wait_grace > time) {
				printf("output closed: %016lx\n", vnic->mac);
				continue;
			}

			Packet* packet = fifo_pop(vnic->nic->output_buffer);
			if(!packet) {
				continue;
			}

			if(vnic->port & 0xfff) {
				if(!vlan_output_process(vnic, packet)) {
					nic_free(packet);
					continue;
				}
			}

			uint16_t size = packet->end - packet->start;
			vnic->nic->output_bytes += size;
			vnic->nic->output_packets++;
			if(vnic->output_closed > time)
				vnic->output_closed += vnic->input_wait * size;
			else
				vnic->output_closed = time + vnic->input_wait * size;


			Ether* ether = (void*)(packet->buffer + packet->start);
			uint64_t dmac = endian48(ether->dmac);

			if(vnic->output_accept && list_index_of(vnic->output_accept, (void*)dmac, NULL) < 0) {
				/* Debuging */
				nic_free(packet);
				// Packet eliminated
				continue;
			}

			if(vnic->mac != 0xffffffffffff)
				ether->smac = endian48(vnic->mac);	// Fix mac address

#if DEBUG
			uint64_t smac = endian48(ether->smac);
			printf("Output: [%02d] %02x:%02x:%02x:%02x:%02x:%02x %02x:%02x:%02x:%02x:%02x:%02x\n",
					local_port,
					(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
					(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
					(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
					(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff);
#endif /* DEBUG */

			return packet;
		}
	}

	return NULL;
}

#if DEBUG
void nic_statistics(uint64_t time) {
/*
 *        printf("\033[0;20HRX: %ld TX: %ld TX: %ld  ", nic_rx, nic_tx2, nic_tx);
 *        nic_rx = nic_tx = nic_tx2 = 0;
 *
 *        MapIterator iter;
 *        map_iterator_init(&iter, nis);
 *        while(map_iterator_has_next(&iter)) {
 *                MapEntry* entry = map_iterator_next(&iter);
 *                VNIC* nic = entry->data;
 *
 *                printf("\033[1;0H%012x %d/%d %d/%d %d/%d    ", nic->mac, nic_pool_used(nic->ni), nic_pool_total(nic->ni), nic->nic->input_packets, nic->nic->output_packets, nic->nic->input_drop_packets, nic->nic->output_drop_packets);
 *        }
 */
}
#endif

