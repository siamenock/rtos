#ifndef __NE_PORT_H__
#define __NE_PORT_H__

#include "node.h"
#include "tun.h"

typedef struct _NEPort NEPort;

struct _NEPort {
	int		type;
	bool	is_active;
	Node*	in;
	Node*	out;
	void	(*send)(Node* this, Packet* packet);
	
//	NI*		ni;
	TunInterface* ti;
	void	(*received)(NEPort* this, Packet* packet);
};

Group* ne_create(int port_count); 
bool ne_destroy(Group* ne);

#endif /* __NE_PORT_H__ */
