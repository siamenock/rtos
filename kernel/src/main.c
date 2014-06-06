#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "asm.h"
#include "gdt.h"
#include "idt.h"
#include "pci.h"
#include "stdio.h"
#include "shell.h"
#include "mp.h"
#include "apic.h"
#include "ioapic.h"
#include "task.h"
#include "cpu.h"
#include "event.h"
#include "shared.h"
#include "malloc.h"
#include "gmalloc.h"
#include "ni.h"
#include "manager.h"
#include "icc.h"
#include "loader.h"
#include "module.h"
#include "symbols.h"
#include "device.h"
#include "driver/dummy.h"
#include "driver/nic.h"

#include <net/ether.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>

#define VGA_BUFFER_PAGES	12

//static uint64_t idle_time;

static Device* nics[8];
static int nics_count;

static void idle0_event(void* data) {
	/*
	static uint64_t tick;
	uint64_t time = cpu_tsc();
	
	if(time > tick) {
		tick = time + cpu_frequency;
		ni_statistics(time);
		printf("\033[0;68HLoad: %3d%%", (cpu_frequency - idle_time) * 100 / cpu_frequency);
		idle_time = 0;
		return;
	}
	*/
	
	// Poll NICs
	int poll_count = 0;
	for(int i = 0; i < nics_count; i++) {
		Device* dev = nics[i];
		NIC* nic = dev->driver;
		poll_count += nic->poll(dev->id);
	}
	
	// idle
	/*
	for(int i = 0; i < 100; i++)
		asm volatile("nop");
	*/
	
	//idle_time += cpu_tsc() - time;
}

static void idle_event(void* data) {
	for(int i = 0; i < 100; i++)
		asm volatile("nop");
}

/*
static void apic_terminate(uint64_t vector, uint64_t errno) {
	apic_eoi();
	
	task_destroy(1);
}
*/

static void icc_start(ICC_Message* msg) {
	printf("Loading process... \n");
	VM* vm = msg->data.start.vm;
	
	// TODO: Change blocks[0] to blocks
	uint32_t id = loader_load(vm);
	if(errno != 0)
		goto failed;
	
	*(uint32_t*)task_addr(id, SYM_NIS_COUNT) = vm->nic_size;
	NetworkInterface** nis = (NetworkInterface**)task_addr(id, SYM_NIS);
	for(uint32_t i = 0; i < vm->nic_size; i++) {
		task_resource(id, RESOURCE_NI, vm->nics[i]);
		nis[i] = vm->nics[i]->ni;
	}
	
	printf("Process executing...\n");
	
	ICC_Message* msg2 = icc_sending(ICC_TYPE_STARTED, msg->core_id);
	msg->status = ICC_STATUS_DONE;
	
	msg2->result = 0;
	msg2->data.execute.stdin = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDIN));
	msg2->data.execute.stdin_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_HEAD));
	msg2->data.execute.stdin_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDIN_TAIL));
	msg2->data.execute.stdin_size = *(int*)task_addr(id, SYM_STDIN_SIZE);
	msg2->data.execute.stdout = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDOUT));
	msg2->data.execute.stdout_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_HEAD));
	msg2->data.execute.stdout_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDOUT_TAIL));
	msg2->data.execute.stdout_size = *(int*)task_addr(id, SYM_STDOUT_SIZE);
	msg2->data.execute.stderr = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)*(char**)task_addr(id, SYM_STDERR));
	msg2->data.execute.stderr_head = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_HEAD));
	msg2->data.execute.stderr_tail = (void*)TRANSLATE_TO_PHYSICAL((uint64_t)task_addr(id, SYM_STDERR_TAIL));
	msg2->data.execute.stderr_size = *(int*)task_addr(id, SYM_STDERR_SIZE);
	
	icc_send(msg2);
	
	APIC_Handler handler14 = apic_register(14, NULL);
	
	void stop14(uint64_t vector, uint64_t err) {
		if(err == 0) {
			apic_eoi();
			
			task_destroy(1);
		} else {
			// TODO: Dump right stack trace
			handler14(vector, err);
		}
	}
	
	void stop49(uint64_t vector, uint64_t err) {
		apic_eoi();
		
		task_destroy(1);
	}
	
	apic_register(14, stop14);
	APIC_Handler handler49 = apic_register(49, stop49);
	
	task_switch(1);
	printf("Execution completed\n");
	
	apic_enable();
	
	apic_register(14, handler14);
	apic_register(49, handler49);
	
	ICC_Message* msg3 = icc_sending(ICC_TYPE_STOPPED, 0);
	icc_send(msg3);
	
	return;

failed:
	msg2 = icc_sending(ICC_TYPE_STARTED, msg->core_id);
	msg->status = ICC_STATUS_DONE;
	
	msg2->result = errno;	// errno from loader_load
	icc_send(msg2);
	printf("Execution FAILED: %x\n", errno);
}


void main(void) {
	mp_init0();
	cpu_init();
	
	// Bootstrap process
	uint8_t core_id = mp_core_id();
	if(core_id == 0) {
		stdio_init();
		
		printf("Initializing shared area...\n");
		shared_init();
		
		printf("Initializing malloc...\n");
		malloc_init();
		
		printf("Initializing gmalloc...\n");
		gmalloc_init();
		stdio_init2(gmalloc(4000 * VGA_BUFFER_PAGES), 4000 * VGA_BUFFER_PAGES);
		
		printf("Analyzing CPU information...\n");
		cpu_info();
		
		printf("Analyzing multi-processors...\n");
		mp_analyze();
		
		printf("Initialize GDT...\n");
		gdt_init();
		gdt_load();
		
		printf("Loading TSS...\n");
		tss_init();
		tss_load();
		
		printf("Initializing IDT...\n");
		idt_init();
		idt_load();
		
		printf("Initializing PCI...\n");
		pci_init();
		
		printf("Initializing APICs...\n");
		apic_activate();
		
		printf("Initailizing multi-processing...\n");
		mp_init();
		
		printf("Initailizing local APIC...\n");
		apic_init();
		
		printf("Initializing I/O APIC...\n");
		ioapic_init();
		apic_enable();
		
		printf("Initializing Multi-tasking...\n");
		task_init();
		
		printf("Initializing inter-core communications...\n");
		icc_init();
		
		printf("Initializing kernel symbols...\n");
		symbols_init();
		
		printf("Initializing modules...\n");
		module_init();
		
		mp_sync();
		
		printf("Initializing network interface...\n");
		ni_init0();
		
		printf("Initializing events...\n");
		event_init();
		event_idle(stdio_event, NULL);
		
		printf("Cleaning up memory...\n");
		gmalloc_extend();
		
		printf("Initializing device drivers: ");
		int count = device_module_init();
		printf("%d inited\n", count);
		
		nics_count = device_count(DEVICE_TYPE_NIC);
		printf("Finding NICs: %d\n", nics_count);
		for(int i = 0; i < count; i++) {
			nics[i] = device_get(DEVICE_TYPE_NIC, i);
			NICStatus status;
			((NIC*)nics[i]->driver)->get_status(nics[i]->id, &status);
			printf("\tNIC[%d]: %02x:%02x:%02x:%02x:%02x:%02x\n", i,
				(status.mac >> 40) & 0xff, 
				(status.mac >> 32) & 0xff, 
				(status.mac >> 24) & 0xff, 
				(status.mac >> 16) & 0xff, 
				(status.mac >> 8) & 0xff, 
				(status.mac >> 0) & 0xff);
			
			if(ni_mac == 0)
				ni_mac = status.mac;
		}
		
		printf("Initializing manager...\n");
		manager_init();
		
		printf("Initializing shell...\n");
		shell_init();
		
		event_busy(idle0_event, NULL);
		
		dummy_init();
	} else {
	// Application Process
		mp_wakeup();
		malloc_init();
		mp_analyze();
		gdt_load();
		tss_load();
		idt_load();
		apic_init();
		apic_enable();
		task_init();
		mp_sync();
		icc_init();
		event_init();
		stdio_init();
		icc_register(ICC_TYPE_START, icc_start);
		
		event_idle(idle_event, NULL);
	}
	
//	if(core_id == 0) {
//		uint64_t attrs[] = { 
//			NI_MAC, 0x001122334455,
//			NI_POOL_SIZE, 0x400000,
//			NI_INPUT_BANDWIDTH, 1000000000L,
//			NI_OUTPUT_BANDWIDTH, 1000000000L,
//			NI_INPUT_BUFFER_SIZE, 1024,
//			NI_OUTPUT_BUFFER_SIZE, 1024,
//			NI_INPUT_ACCEPT_ALL, 1,
//			NI_OUTPUT_ACCEPT_ALL, 1,
//			NI_NONE
//		};
//		
//		NI* ni = ni_create(attrs);
//		printf("NI: %p\n", ni);
//	}
//	
//	if(core_id == 1) {
//		cpu_swait(2);
//		NI* ni = (void*)0xce6cc140;
//		printf("NI.mac = %x\n", ni->mac);
//		uint32_t myip = 0xc0a8c864;
//		
//		void core1_io(void* data) {
//			NetworkInterface* ni = data;
//			
//			if(!ni_has_input(ni)) {
////				int packet_size = ETHER_LEN + 64;
////				Packet* packet = ni_alloc(ni, packet_size);
////				
////				packet->end = packet->start + packet_size;
////				Ether* ether = (Ether*)(packet->buffer + packet->start);
////				IP* ip = (IP*)ether->payload;
////				UDP* udp = (UDP*)ip->body;
////				
////				ether->dmac = endian48(0x94de806314a9);
////				ether->smac = endian48(ni->mac);
////				ether->type = endian16(ETHER_TYPE_IPv4);
////				
////				ip->version = endian8(4);
////				ip->ihl = endian8(IP_LEN / 4);
////				ip->dscp = 0;
////				ip->ecn = 0;
////				ip->length = endian16(packet_size - ETHER_LEN);
////				ip->id = cpu_tsc() & 0xffff;
////				ip->flags_offset = endian16(0x02 << 13);
////				ip->ttl = endian8(0x40);
////				ip->protocol = endian8(IP_PROTOCOL_UDP);
////				ip->source = endian32(myip);
////				ip->destination = endian32(myip + 100);
////				ip->checksum = 0;
////				ip->checksum = endian16(checksum(ip, ip->ihl * 4));
////				
////				udp->source = endian16(7);
////				udp->destination= endian16(8000);
////				udp->length = endian16(packet_size - ETHER_LEN - IP_LEN);
////				udp->checksum = 0;
////				
////				ni_output(ni, packet);
//				return;
//			}
//			
//			Packet* packet = ni_input(ni);
//			Ether* ether = (Ether*)(packet->buffer + packet->start);
//			if(endian16(ether->type) == ETHER_TYPE_ARP) {
//				// ARP response
//				ARP* arp = (ARP*)ether->payload;
//				if(endian16(arp->operation) == 1 && endian32(arp->tpa) == myip) {
//					ether->dmac = ether->smac;
//					ether->smac = endian48(ni->mac);
//					arp->operation = endian16(2);
//					arp->tha = arp->sha;
//					arp->tpa = arp->spa;
//					arp->sha = ether->smac;
//					arp->spa = endian32(myip);
//					
//					ni_output(ni, packet);
//					packet = NULL;
//				}
//			} else if(endian16(ether->type) == ETHER_TYPE_IPv4) {
//				IP* ip = (IP*)ether->payload;
//				
//				if(ip->protocol == IP_PROTOCOL_ICMP && endian32(ip->destination) == myip) {
//					// Echo reply
//					ICMP* icmp = (ICMP*)ip->body;
//					
//					icmp->type = 0;
//					icmp->checksum = 0;
//					icmp->checksum = endian16(checksum(icmp, packet->end - packet->start - ETHER_LEN - IP_LEN));
//					
//					ip->destination = ip->source;
//					ip->source = endian32(myip);
//					ip->ttl = endian8(64);
//					ip->checksum = 0;
//					ip->checksum = endian16(checksum(ip, ip->ihl * 4));
//					
//					ether->dmac = ether->smac;
//					ether->smac = endian48(ni->mac);
//					
//					ni_output(ni, packet);
//					packet = NULL;
//				} else if(ip->protocol == IP_PROTOCOL_UDP) {
//					UDP* udp = (UDP*)ip->body;
//					if(endian16(udp->destination) == 7) {	// Echo service
//						uint16_t t = udp->destination;
//						udp->destination = udp->source;
//						udp->source = t;
//						//udp->destination = endian16(1024);	// For test purpose
//						udp->checksum = 0;
//						
//						uint32_t t2 = ip->destination;
//						ip->destination = ip->source;
//						ip->source = t2;
//						ip->ttl = 0x40;
//						ip->checksum = 0;
//						ip->checksum = endian16(checksum(ip, ip->ihl * 4));
//
//						uint64_t t3 = ether->dmac;
//						ether->dmac = ether->smac;
//						ether->smac = t3;
//
//						ni_output(ni, packet);
//						packet = NULL;
//					}
//				}
//			}
//			
//			if(packet)
//				ni_free(packet);
//		}
//		
//		event_busy((void*)core1_io, ni->ni);
//	}
	
	mp_sync();
	
	while(1)
		event_loop();
}
