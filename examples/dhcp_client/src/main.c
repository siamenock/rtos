#include <stdio.h>
#include <string.h>
#include <readline.h>
#include <thread.h>
#include <net/nic.h>
#include <net/dhcp.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/ether.h>
#include <util/event.h>

bool discovered_callback(NIC* nic, uint32_t transaction_id, uint32_t ip, void* data) {
	printf("dicovered_callback \n\n");
	return true;
}

bool ack_received_callback(NIC* nic, uint32_t transaction_id, uint32_t ip, void* data) {
	dhcp_bound(nic, transaction_id);
	return true;
}

bool offered_callback(NIC* nic, uint32_t transaction_id, uint32_t ip, void* data) {
	dhcp_request(nic, transaction_id);
	return true;
}

void ginit(int argc, char** argv) {
	uint32_t i;
	uint32_t count = nic_count();
	for(i=0; count > i; i++) {
		NIC* nic = nic_get(i);
		dhcp_init(nic);
		printf("dhcp_init\n");
	}
}

void init(int argc, char** argv) {
	event_init();
}

void process(NIC* ni) {
	Packet* packet = nic_input(ni);
	if(!packet)
		return;
	dhcp_process(packet);
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
		event_loop();
			char* line = readline();
			if(line) { 
				if(!strcmp(line, "discover")) {
					dhcp_create_session(nic, discovered_callback,
						 offered_callback, ack_received_callback, NULL); 
				}
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

