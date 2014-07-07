#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <time.h>
#include <rpc/xdr.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <rpc/auth.h>

#include <net/ether.h>
#include <net/arp.h>
#undef IP_TTL
#include <net/ip.h>
#include <net/udp.h>
#include <net/checksum.h>
#include <net/ni.h>
#include <util/fifo.h>
#include <util/map.h>

typedef struct {
	uint32_t		ip;
	uint16_t		port;
	unsigned long		prognum;
	unsigned long		versnum;
} ClientPrivate;

typedef struct {
	uint32_t		xid;
	xdrproc_t		outproc;
	char*			out;
	struct timeval		tout;
	
	void			(*succeed)(void* context, void* result);
	void			(*failed)(void* context, int stat1, int stat2);
	void			(*timeout)(void* context);
	void*			context;
} Call;

static SVCXPRT xprt;
struct xp_ops ops;

// TODO: Below in SVCXPRT
static Map* services;
static Map* calls;
static FIFO* history_fifo;
static Map* history_map;

static bool_t xdr_msg_type (XDR *xdrs, enum msg_type *objp) {
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_reply_stat (XDR *xdrs, enum reply_stat *objp) {
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_accept_stat (XDR *xdrs, enum accept_stat *objp) {
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_reject_stat (XDR *xdrs, enum reject_stat *objp) {
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_auth_stat (XDR *xdrs, enum auth_stat *objp) {
	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_call_body (XDR *xdrs, struct call_body *objp) {
	register int32_t *buf;

	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE (xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->cb_rpcvers))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_prog))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_vers))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_proc))
				 return FALSE;

		} else {
		IXDR_PUT_U_LONG(buf, objp->cb_rpcvers);
		IXDR_PUT_U_LONG(buf, objp->cb_prog);
		IXDR_PUT_U_LONG(buf, objp->cb_vers);
		IXDR_PUT_U_LONG(buf, objp->cb_proc);
		}
		 if (!xdr_opaque_auth (xdrs, &objp->cb_cred))
			 return FALSE;
		 if (!xdr_opaque_auth (xdrs, &objp->cb_verf))
			 return FALSE;
		return TRUE;
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE (xdrs, 4 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			 if (!xdr_u_long (xdrs, &objp->cb_rpcvers))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_prog))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_vers))
				 return FALSE;
			 if (!xdr_u_long (xdrs, &objp->cb_proc))
				 return FALSE;

		} else {
		objp->cb_rpcvers = IXDR_GET_U_LONG(buf);
		objp->cb_prog = IXDR_GET_U_LONG(buf);
		objp->cb_vers = IXDR_GET_U_LONG(buf);
		objp->cb_proc = IXDR_GET_U_LONG(buf);
		}
		 if (!xdr_opaque_auth (xdrs, &objp->cb_cred))
			 return FALSE;
		 if (!xdr_opaque_auth (xdrs, &objp->cb_verf))
			 return FALSE;
	 return TRUE;
	}

	 if (!xdr_u_long (xdrs, &objp->cb_rpcvers))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->cb_prog))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->cb_vers))
		 return FALSE;
	 if (!xdr_u_long (xdrs, &objp->cb_proc))
		 return FALSE;
	 if (!xdr_opaque_auth (xdrs, &objp->cb_cred))
		 return FALSE;
	 if (!xdr_opaque_auth (xdrs, &objp->cb_verf))
		 return FALSE;
	return TRUE;
}

static bool_t xdr_accepted_reply (XDR *xdrs, struct accepted_reply *objp) {
	 if (!xdr_opaque_auth (xdrs, &objp->ar_verf))
		 return FALSE;
	 if (!xdr_accept_stat (xdrs, &objp->ar_stat))
		 return FALSE;
	switch (objp->ar_stat) {
	case SUCCESS:
		if(xdrs->x_op == XDR_ENCODE) {
			 if (!objp->ru.AR_results.proc (xdrs, objp->ru.AR_results.where))
				 return FALSE;
		}
		break;
	case PROG_MISMATCH:
		 if (!xdr_u_long(xdrs, &objp->ru.AR_versions.low))
			 return FALSE;
		 if (!xdr_u_long(xdrs, &objp->ru.AR_versions.high))
			 return FALSE;
		break;
	case PROG_UNAVAIL:
	case PROC_UNAVAIL:
	case GARBAGE_ARGS:
	case SYSTEM_ERR:
		return TRUE;
	default:
		return FALSE;
	}
	return TRUE;
}

static bool_t xdr_rejected_reply (XDR *xdrs, struct rejected_reply *objp) {
	 if (!xdr_reject_stat (xdrs, &objp->rj_stat))
		 return FALSE;
	switch (objp->rj_stat) {
	case RPC_MISMATCH:
		 if (!xdr_u_long(xdrs, &objp->ru.RJ_versions.low))
			 return FALSE;
		 if (!xdr_u_long(xdrs, &objp->ru.RJ_versions.high))
			 return FALSE;
		break;
	case AUTH_ERROR:
		 if (!xdr_auth_stat (xdrs, &objp->ru.RJ_why))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static bool_t xdr_reply_body (XDR *xdrs, struct reply_body *objp) {
	 if (!xdr_reply_stat (xdrs, &objp->rp_stat))
		 return FALSE;
	switch (objp->rp_stat) {
	case MSG_ACCEPTED:
		 if (!xdr_accepted_reply (xdrs, &objp->ru.RP_ar))
			 return FALSE;
		break;
	case MSG_DENIED:
		 if (!xdr_rejected_reply (xdrs, &objp->ru.RP_dr))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

static bool_t xdr_rpc_msg (XDR *xdrs, struct rpc_msg *objp) {
	 if (!xdr_u_long (xdrs, &objp->rm_xid))
		 return FALSE;
	 if (!xdr_msg_type (xdrs, &objp->rm_direction))
		 return FALSE;
	switch (objp->rm_direction) {
	case CALL:
		 if (!xdr_call_body (xdrs, &objp->ru.RM_cmb))
			 return FALSE;
		break;
	case REPLY:
		 if (!xdr_reply_body (xdrs, &objp->ru.RM_rmb))
			 return FALSE;
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

bool_t xdr_opaque_auth (XDR *xdrs, struct opaque_auth *objp) {
	 if (!xdr_enum (xdrs, &objp->oa_flavor))
		 return FALSE;
	 if (!xdr_bytes (xdrs, &objp->oa_base, &objp->oa_length, MAX_AUTH_BYTES))
		 return FALSE;
	return TRUE;
}

static bool_t sendreply(SVCXPRT *xprt, struct rpc_msg* msg) {
	Packet* packet = (void*)xprt->xp_p2;
	
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	IP* ip = (IP*)ether->payload;
	UDP* udp = (UDP*)ip->body;
	
	XDR xdr;
	xdrmem_create(&xdr, (char*)udp->body, packet->buffer + packet->size - udp->body, XDR_ENCODE);
	
	memcpy(&msg->rm_xid, udp->body, sizeof(msg->rm_xid));
	msg->rm_xid = endian32(msg->rm_xid);
	
	if(!xdr_rpc_msg(&xdr, msg)) {
		svcerr_systemerr(xprt);
		return FALSE;
	}
	int len = UDP_LEN + xdr_getpos(&xdr);
	xdr_destroy(&xdr);
	
	swap16(udp->source, udp->destination);
	udp->length = endian16(len);
	udp->checksum = 0;
	packet->end = ((void*)udp + len) - (void*)packet->buffer;
	
	swap32(ip->source, ip->destination);
	ip->ttl = 64;
	ip->length = endian16(ip->ihl * 4 + len);
	ip->checksum = 0;
	ip->checksum = endian16(checksum(ip, ip->ihl * 4));
	
	swap48(ether->smac, ether->dmac);
	
	ni_output_dup(packet->ni, packet);
	
	map_update(history_map, (void*)(uint64_t)msg->rm_xid, packet);
	
	return TRUE;
}

bool_t svc_sendreply(SVCXPRT *xprt, xdrproc_t outproc, char *out) {
	struct rpc_msg msg;
	msg.rm_direction = REPLY;
	msg.ru.RM_rmb.rp_stat = MSG_ACCEPTED;
	msg.ru.RM_rmb.ru.RP_ar.ar_verf = xprt->xp_verf;
	msg.ru.RM_rmb.ru.RP_ar.ar_stat = SUCCESS;
	msg.ru.RM_rmb.ru.RP_ar.ru.AR_results.where = (caddr_t)out;
	msg.ru.RM_rmb.ru.RP_ar.ru.AR_results.proc = outproc;
	
	return sendreply(xprt, &msg);
}

void svcerr_noproc(SVCXPRT *xprt) {
	struct rpc_msg msg;
	
	msg.rm_direction = REPLY;
	msg.ru.RM_rmb.rp_stat = MSG_ACCEPTED;
	msg.ru.RM_rmb.ru.RP_ar.ar_verf = xprt->xp_verf;
	msg.ru.RM_rmb.ru.RP_ar.ar_stat = PROC_UNAVAIL;
	
	sendreply(xprt, &msg);
}

void svcerr_decode(SVCXPRT *xprt) {
	struct rpc_msg msg;
	
	msg.rm_direction = REPLY;
	msg.ru.RM_rmb.rp_stat = MSG_ACCEPTED;
	msg.ru.RM_rmb.ru.RP_ar.ar_verf = xprt->xp_verf;
	msg.ru.RM_rmb.ru.RP_ar.ar_stat = GARBAGE_ARGS;
	
	sendreply(xprt, &msg);
}

void svcerr_systemerr(SVCXPRT *xprt) {
	struct rpc_msg msg;
	
	msg.rm_direction = REPLY;
	msg.ru.RM_rmb.rp_stat = MSG_ACCEPTED;
	msg.ru.RM_rmb.ru.RP_ar.ar_verf = xprt->xp_verf;
	msg.ru.RM_rmb.ru.RP_ar.ar_stat = SYSTEM_ERR;
	
	sendreply(xprt, &msg);
}

typedef struct {
	unsigned long prognum;
	unsigned long versnum;
	void (*dispatch)(struct svc_req *, SVCXPRT *);
	unsigned long protocol;
} Service;

static uint64_t service_hash(void* key) {
	Service* service = key;
	return service->prognum + service->versnum;
}

static bool service_equals(void* key1, void* key2) {
	Service* service1 = key1;
	Service* service2 = key2;
	
	return service1->prognum == service2->prognum &&
		service1->versnum == service2->versnum;
}

bool_t pmap_unset(unsigned long prognum, unsigned long versnum) {
	if(!services)
		return FALSE;
	
	Service key;
	key.prognum = prognum;
	key.versnum = versnum;
	
	Service* service = map_get(services, &key);
	if(service) {
		free(service);
		return TRUE;
	}
	
	return FALSE;
}

static bool_t getargs(SVCXPRT *xprt, xdrproc_t inproc, char *in) {
	XDR* xdr = (void*)xprt->xp_p1;
	return inproc(xdr, in);
}

static bool_t freeargs(SVCXPRT *xprt, xdrproc_t inproc, char *in) {
	XDR* xdr = (void*)xprt->xp_p1;
	xdr->x_op = XDR_FREE;
	return inproc(xdr, in);
}

// TODO: Create SVCXPRT dynamically
SVCXPRT *svcudp_create(int sock) {
	if(!services) {
		services = map_create(4, service_hash, service_equals, malloc, free);
	}
	
	if(!history_fifo) {
		history_fifo = fifo_create(16, malloc, free);
	}
	
	if(!history_map) {
		history_map = map_create(15, map_uint64_hash, map_uint64_equals, malloc, free);
	}
	
	ops.xp_getargs = getargs;
	ops.xp_freeargs = freeargs;
	xprt.xp_ops = &ops;
	
	return &xprt;
}

bool_t svc_register(SVCXPRT *xprt, unsigned long prognum, unsigned long versnum, 
		void (*dispatch)(struct svc_req *, SVCXPRT *), unsigned long protocol) {
	
	if(!services)
		return FALSE;
	
	Service* service = malloc(sizeof(Service));
	bzero(service, sizeof(Service));
	
	service->prognum = prognum;
	service->versnum = versnum;
	service->dispatch = dispatch;
	service->protocol = protocol;
	
	map_put(services, service, service);
	
	return TRUE;
}

bool rpc_process(Packet* packet) {
	if(!packet->ni->config)
		return false;
	
	uint32_t addr = (uint32_t)(uint64_t)map_get(packet->ni->config, "ip");
	if(!addr)
		return false;
	
	// Parse packet
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	if(endian16(ether->type) != ETHER_TYPE_IPv4)
		return false;
	
	IP* ip = (IP*)ether->payload;
	if(endian32(ip->destination) != addr || ip->protocol != IP_PROTOCOL_UDP)
		return false;
	
	UDP* udp = (UDP*)ip->body;
	if(endian16(udp->destination) != 111)
		return false;
	
	// Parse message
	XDR xdr;
	xdrmem_create(&xdr, (char*)udp->body, endian16(udp->length) - UDP_LEN, XDR_DECODE);
	
	struct rpc_msg msg;
	if(!xdr_rpc_msg(&xdr, &msg)) {
		ni_free(packet);
		return true;
	}
	
	if(msg.rm_direction == CALL) {
		// Duplicated XID
		if(map_contains(history_map, (void*)(uint64_t)msg.rm_xid)) {
			Packet* back = map_get(history_map, (void*)(uint64_t)msg.rm_xid);
			
			if(back) {
				ni_output_dup(back->ni, back);
			}
			
			ni_free(packet);
			
			return true;
		}
		
		// Make a room for history
		if(fifo_size(history_fifo) >= 15) {
			uint64_t xid = (uint64_t)fifo_pop(history_fifo);
			Packet* back = map_remove(history_map, (void*)xid);
			if(back)
				ni_free(back);
		}
		
		// Add history
		fifo_push(history_fifo, (void*)(uint64_t)msg.rm_xid);
		map_put(history_map, (void*)(uint64_t)msg.rm_xid, NULL);
		
		if(msg.ru.RM_cmb.cb_rpcvers != 2)
			return false;
		
		// Find service
		Service key;
		key.prognum = msg.ru.RM_cmb.cb_prog;
		key.versnum = msg.ru.RM_cmb.cb_vers;
		
		Service* service = map_get(services, &key);
		if(!service)
			return false;
		
		// Make svc_req
		struct svc_req req;
		req.rq_prog = msg.ru.RM_cmb.cb_prog;
		req.rq_vers = msg.ru.RM_cmb.cb_vers;
		req.rq_proc = msg.ru.RM_cmb.cb_proc;
		req.rq_clntcred = (caddr_t)&msg.ru.RM_cmb.cb_cred;
		req.rq_xprt = &xprt;
		
		// Set svc xprt
		xprt.xp_verf = msg.ru.RM_cmb.cb_verf;
		xprt.xp_p1 = (caddr_t)&xdr;
		xprt.xp_p2 = (caddr_t)packet;
		
		service->dispatch(&req, &xprt);
		
		return true;
	} else if(calls && msg.rm_direction == REPLY) {
		Call* call = map_remove(calls, (void*)(uint64_t)msg.rm_xid);
		if(!call) {
			ni_free(packet);
			return true;
		}
		
		int stat1 = msg.ru.RM_rmb.rp_stat;
		int stat2 = 0;
		if(msg.ru.RM_rmb.rp_stat == MSG_ACCEPTED) {
			stat2 = msg.ru.RM_rmb.ru.RP_ar.ar_stat;
			if(msg.ru.RM_rmb.ru.RP_ar.ar_stat == SUCCESS) {
				if(call->outproc(&xdr, call->out)) {
					if(call->succeed)
						call->succeed(call->context, call->out);
					goto done;
				}
			}
		} else {
			stat2 = msg.ru.RM_rmb.ru.RP_dr.rj_stat;
		}
		
		if(call->failed)
			call->failed(call->context, stat1, stat2);
		
done:
		free(call);
		
		return true;
	} else {	// Illegal RPC packet
		ni_free(packet);
		return true;
	}
}

static void cl_destroy(CLIENT* clnt) {
	free(clnt);
}

struct clnt_ops cl_ops = {
	.cl_destroy = cl_destroy
};

CLIENT* rpc_client(uint32_t ip, uint16_t port, unsigned long prognum, unsigned long versnum) {
	CLIENT* clnt = malloc(sizeof(CLIENT) + sizeof(ClientPrivate));
	if(!clnt)
		return NULL;
	bzero(clnt, sizeof(CLIENT) + sizeof(ClientPrivate));
	
	ClientPrivate* priv = (void*)clnt + sizeof(CLIENT);
	priv->ip = ip;
	priv->port = port;
	priv->prognum = prognum;
	priv->versnum = versnum;
	
	clnt->cl_ops = &cl_ops;
	clnt->cl_private = (caddr_t)priv;
	
	return clnt;
}

bool rpc_call(NetworkInterface* ni, CLIENT* client, unsigned long procnum, xdrproc_t inproc, char* in, xdrproc_t outproc, char* out, struct timeval tout, void(*succeed)(void* context, void* result), void(*failed)(void* context, int stat1, int stat2), void(*timeout)(void* context), void* context) {
	// TODO: timeout using event
	
	ClientPrivate* priv = (void*)client + sizeof(CLIENT);
	
	Packet* packet = ni_alloc(ni, 1500);
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	ether->dmac = endian48(arp_get_mac(ni, priv->ip));
	ether->smac = endian48(ni->mac);
	ether->type = endian16(ETHER_TYPE_IPv4);
	
	IP* ip = (IP*)ether->payload;
	ip->version = endian8(4);
	ip->ihl = endian8(IP_LEN / 4);
	ip->dscp = 0;
	ip->ecn = 0;
	ip->length = 0;//endian16(packet_size - ETHER_LEN);
	ip->id = clock() & 0xffff;
	ip->flags_offset = endian16(0x02 << 13);
	ip->ttl = endian8(0x40);
	ip->protocol = endian8(IP_PROTOCOL_UDP);
	ip->source = endian32((uint32_t)(uint64_t)map_get(ni->config, "ip"));
	ip->destination = endian32(priv->ip);
	ip->checksum = 0;
	
	UDP* udp = (UDP*)ip->body;
	udp->source = endian16(111);
	udp->destination = endian16(priv->port);
	udp->checksum = 0;
	
	XDR xdr;
	xdrmem_create(&xdr, (char*)udp->body, packet->buffer + packet->size - udp->body, XDR_ENCODE);
	
	struct rpc_msg msg;
	msg.rm_xid = (uint32_t)clock();
	msg.rm_direction = CALL;
	msg.ru.RM_cmb.cb_rpcvers = 2;
	msg.ru.RM_cmb.cb_prog = priv->prognum;
	msg.ru.RM_cmb.cb_vers = priv->versnum;
	msg.ru.RM_cmb.cb_proc = procnum;
	bzero(&msg.ru.RM_cmb.cb_cred, sizeof(struct opaque_auth));
	bzero(&msg.ru.RM_cmb.cb_verf, sizeof(struct opaque_auth));
	
	if(!xdr_rpc_msg(&xdr, &msg)) {
		ni_free(packet);
		return false;
	}
	
	if(!inproc(&xdr, in)) {
		ni_free(packet);
		return false;
	}
	
	udp_pack(packet, xdr_getpos(&xdr));
	xdr_destroy(&xdr);
	
	ni_output(ni, packet);
	
	Call* call = malloc(sizeof(Call));
	bzero(call, sizeof(Call));
	call->xid = msg.rm_xid;
	call->outproc = outproc;
	call->out = out;
	call->tout = tout;
	call->succeed = succeed;
	call->failed = failed;
	call->timeout = timeout;
	call->context = context;
	
	if(!calls) {
		calls = map_create(16, map_uint64_hash, map_uint64_equals, malloc, free);
	}
	
	map_put(calls, (void*)(uint64_t)call->xid, call);
	
	return true;
}
