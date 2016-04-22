#include <stdio.h>
#include <thread.h>
#include <net/nic.h>

void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

void process(NIC* ni) {
	Packet* packet = nic_input(ni);
	if(!packet)
		return;
	
	if(packet)
		nic_free(packet);
}

void destroy() {
}

void gdestroy() {
}

int main(int argc, char** argv) {
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
			
			NIC* ni = nic_get(i);
			if(nic_has_input(ni)) {
				process(ni);
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
