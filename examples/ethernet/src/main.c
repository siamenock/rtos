#include <stdio.h>
#include <thread.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

void process(NIC* nic) {
	Packet* packet = nic_input(nic);
	if(!packet)
		return;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	uint64_t dmac = endian48(ether->dmac);
	uint64_t smac = endian48(ether->smac);
	uint64_t type = endian16(ether->type);
	
	printf("dmac: %02x:%02x:%02x:%02x:%02x:%02x "
		"smac: %02x:%02x:%02x:%02x:%02x:%02x "
		"type: %04x payload: %d\n", 
		(dmac >> 40) & 0xff, (dmac >> 32) & 0xff, (dmac >> 24) & 0xff,
		(dmac >> 16) & 0xff, (dmac >> 8) & 0xff, (dmac >> 0) & 0xff,
		(smac >> 40) & 0xff, (smac >> 32) & 0xff, (smac >> 24) & 0xff,
		(smac >> 16) & 0xff, (smac >> 8) & 0xff, (smac >> 0) & 0xff,
		type, packet->end - packet->start - sizeof(Ether));
	
	if(packet)
		nic_free(packet);
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	uint32_t i = 0;
	while(1) {
		uint32_t count = nic_count();
		if(count > 0) {
			i = (i + 1) % count;
			
			NIC* nic = nic_get(i);
			if(nic_has_input(nic)) {
				process(nic);
			}
		}
	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
