#ifndef __NODE_H__
#define __NODE_H__

#include <stddef.h>
#include <stdbool.h>
#include <net/packet.h>

enum {
	NODE_TYPE_NONE,
	NODE_TYPE_NE,
	NODE_TYPE_NE_PORT,
	NODE_TYPE_LINK,
	NODE_TYPE_LINE,
	NODE_TYPE_SWITCH,
	NODE_TYPE_SWITCH_PORT,
	NODE_TYPE_END
} NodeType;

typedef struct _Node Node;

struct _Node {
	int		type;
	bool	is_active;
	Node*	in;
	Node*	out;
	void	(*send)(Node* this, Packet* packet);
};

typedef struct _Group {
	int		type;
	bool	is_active;
	
	int		node_count;
	Node**	nodes;
} Group;

#endif /* __NODE_H__ */
