#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "ne.h"

static void send(Node* this, Packet* packet) {
	printf("NE send to %s\n", ((NEPort*)this)->ti->name);

	if(((NEPort*)this)->ti)
		write(((NEPort*)this)->ti->fd, packet->buffer + packet->start , packet->end - packet->start); 
	
	//free(packet);
}

static void received(NEPort* this, Packet* packet) { 
	printf("NE received from %s\n", ((NEPort*)this)->ti->name);
	if(this->out) {
		this->out->send(this->out, packet);
	} else {
		printf("Packet dropped at NE port\n"); // TODO: drop description  
		free(packet);
	}
}

Group* ne_create(int port_count) { 
	Group* ne = (Group*)malloc(sizeof(Group));
	if(!ne) {
		printf("NE create malloc error\n");
		return NULL;
	}
	memset(ne, 0x0, sizeof(Group));

	ne->nodes = (Node**)malloc(sizeof(Node*) * port_count);
	if(!ne->nodes) {
		printf("NE create malloc error\n");
		return NULL;
	}
	memset(ne->nodes, 0x0, sizeof(Node*) * port_count);

	ne->type = NODE_TYPE_NE;
	ne->is_active = true;
	ne->node_count = port_count;

	NEPort* ne_port;
	for(int i = 0, k = 0; i < port_count; i++) {
		ne_port = (NEPort*)malloc(sizeof(NEPort));
		
		if(!ne_port) {
			printf("NE port malloc error\n");
			return NULL;
		}
		memset(ne_port, 0x0, sizeof(NEPort));
		// mapping port to tap interface
		ne_port->type = NODE_TYPE_NE_PORT;

		char name[16];
		sprintf(name, "tap%d", k + 100);
		while(!(ne_port->ti = tun_create(name, IFF_TAP | IFF_NO_PI))) {
			k++;
			sprintf(name, "tap%d", k + 100);
		}

		ne_port->send = send;
		ne_port->received = received;
		ne_port->out = NULL;
	
		ne->type = NODE_TYPE_NE;
		ne->nodes[i] = (Node*)ne_port;
		ne->node_count = port_count;
	}
	
	printf("Network Emulator created\n");
	return ne;
}

bool ne_destroy(Group* ne) {
	for(int i = 0; ne->nodes[i] != NULL; i++) {
		tun_destroy(((NEPort*)ne->nodes[i])->ti);
		free(ne->nodes[i]);
		ne->nodes[i] = NULL;
		i++;
	}
	free(ne);
	
	printf("Network Emulator destroyed\n");
	return true;
}



	
