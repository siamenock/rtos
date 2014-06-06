#include <string.h>
#include <stdlib.h>
#include <util/map.h>

#undef BYTE_ORDER
#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/sys.h>
#include <netif/etharp.h>

#include "httpd.h"

#define METHOD_POST	1
#define METHOD_GET	2
#define METHOD_PUT	3
#define METHOD_DELETE	4

#define HTTP_1_0	1
#define HTTP_1_1	2

#define PATH_MAX_LEN	1024

struct _HTTPD {
	Map*	post;
	Map*	get;
	Map*	put;
	Map*	delete;
};

#define BUFFER_SIZE	0x8000	// 8K

typedef struct {
	char*		path;
	HTTPCallback	callback;
} Callback;

typedef struct _Conn {
	HTTPD*		httpd;
	
	int		method;
	int		version;
	char		path[PATH_MAX_LEN];
	
	HTTPContext	req;
	HTTPContext	res;
	
	List*		close_listeners;
	List*		send_listeners;
	
	void*		pcb;
} Conn;

typedef struct {
	bool(*listener)(HTTPContext* req, HTTPContext* res, void* context);
	void* context;
} Listener;

static void close(Conn* conn, struct tcp_pcb* pcb, struct pbuf* buf) {
	if(buf)
		pbuf_free(buf);
	
	ListIterator iter;
	list_iterator_init(&iter, conn->close_listeners);
	while(list_iterator_has_next(&iter)) {
		Listener* l = list_iterator_remove(&iter);
		
		l->listener(&conn->req, &conn->res, l->context);
		free(l);
	}
	list_destroy(conn->close_listeners);
	conn->close_listeners = NULL;
	
	list_iterator_init(&iter, conn->send_listeners);
	while(list_iterator_has_next(&iter)) {
		Listener* l = list_iterator_remove(&iter);
		free(l);
	}
	list_destroy(conn->send_listeners);
	conn->send_listeners = NULL;
	
	httpd_param_clear(&conn->req);
	map_destroy(conn->req.params);
	conn->req.params = NULL;
	httpd_param_clear(&conn->res);
	map_destroy(conn->res.params);
	conn->res.params = NULL;
	httpd_header_clear(&conn->req);
	map_destroy(conn->req.headers);
	conn->req.headers = NULL;
	httpd_header_clear(&conn->res);
	map_destroy(conn->res.headers);
	conn->res.headers = NULL;
	
	free(conn);
	
	tcp_arg(pcb, NULL);
	tcp_poll(pcb, NULL, 0);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_close(pcb);
}

static err_t parse_header(Conn* conn) {
	char* c = (char*)conn->req.buf.buf;
	char* p = strchr(c, ' ');
	if(p < c)
		return -1;
	
	// Parse method
	int len = p - c;
	if(len == 4 && strncmp(c, "POST", len) == 0) {
		conn->method = METHOD_POST;
	} else if(len == 3 && strncmp(c, "GET", len) == 0) {
		conn->method = METHOD_GET;
	} else if(len == 3 && strncmp(c, "PUT", len) == 0) {
		conn->method = METHOD_PUT;
	} else if(len == 6 && strncmp(c, "DELETE", len) == 0) {
		conn->method = METHOD_DELETE;
	} else {
		return -1;
	}
	
	// Parse path
	c = p + 1;
	p = strchr(c, ' ');
	if(p < c)
		return -1;
	
	int path_len = p - c;
	if(path_len >= PATH_MAX_LEN)
		return -1;
	
	memcpy(conn->path, c, path_len);
	conn->path[path_len] = '\0';
	
	// Parse version
	c = p + 1;
	if(strncmp(c, "HTTP/1.0\r\n", 10) == 0) {
		conn->version = HTTP_1_0;
		c += 10;
	} else if(strncmp(c, "HTTP/1.1\r\n", 10) == 0) {
		conn->version = HTTP_1_1;
		c += 10;
	} else {
		return -1;
	}
	
	// Parse extra headers
	while(strncmp(c, "\r\n", 2) != 0) {
		char* key = c;
		p = strchr(c, ':');
		*p = '\0';
		
		c = p + 1;
		while(*c == ' ')
			c++;
		
		char* value = c;
		char* p = strstr(c, "\r\n");
		*p = '\0';
		
		httpd_header_put(&conn->req, key, value);
		
		c = p + 2;
	}
	
	return ERR_OK;
}

static void parse_chunk(HTTPContext* ctx) {
	uint8_t* find_eol(uint8_t* buf, uint8_t* end) {
		for( ; buf <= end - 2; buf++) {
			if(buf[0] == '\r' && buf[1] == '\n')
				return buf + 2;
		}
		return NULL;
	}
	
	uint8_t* end = ctx->buf.buf + ctx->buf.index;
	uint8_t* buf = end - ctx->buf.total_index + ctx->buf.total_length;
	
	while(buf < end) {
		// Parse content length
		uint8_t* content = find_eol(buf, end);
		if(!content)
			return;
		
		long length = strtol((const char*)buf, NULL, 16);
		if(end - content - 2 < length) {	// Wait for content
			return;
		}
		
		int len0 = content - buf;
		ctx->buf.index -= len0 + 2;
		ctx->buf.total_index -= len0 + 2;
		ctx->buf.total_length += length;
		
		memmove(buf, content, end - content);
		buf += length;
		end -= len0;
		
		memmove(buf, buf + 2, end - (buf + 2));
		end -= 2;
	}
}

static err_t recv_content(Conn* conn, struct tcp_pcb* pcb, struct pbuf* buf) {
	if(conn->req.buf.index + buf->tot_len > HTTP_BUFFER_SIZE) {
		// Reiceve buffer overflow
		return -1;
	}
	
	memcpy(conn->req.buf.buf + conn->req.buf.index, buf->payload, buf->tot_len);
	conn->req.buf.index += buf->tot_len;
	conn->req.buf.total_index += buf->tot_len;
	
	switch(conn->res.encoding) {
		case HTTP_ENCODING_IDENTITY:
			// Do nothing
			break;
		case HTTP_ENCODING_CHUNKED:
			parse_chunk(&conn->req);
			break;
	}
	
	return ERR_OK;
}

static err_t recv_header(Conn* conn, struct tcp_pcb* pcb, struct pbuf* buf) {
	// Check the available buffer size
	if(conn->req.buf.index + buf->tot_len > HTTP_BUFFER_SIZE) {
		return -1;	// Header size exceeds request buffer size
	}
	
	// Copy
	memcpy(conn->req.buf.buf + conn->req.buf.index, buf->payload, buf->tot_len);
	int i = conn->req.buf.index - 4;
	if(i < 0)
		i = 14;	// Skip the first line of HTTP request
	
	conn->req.buf.index += buf->tot_len;
	
	// Check end of header
	char* c = (char*)conn->req.buf.buf;
	for(; i < conn->req.buf.index - 3; i++) {
		if(c[i] == '\r' && c[i + 1] == '\n' && c[i + 2] == '\r' && c[i + 3] == '\n') {
			err_t err = parse_header(conn);
			if(err != ERR_OK) {
				return -2;	// Header format error
			}
			
			int header_len = i + 4;
			memmove(conn->req.buf.buf, conn->req.buf.buf + header_len, conn->req.buf.index - header_len);
			conn->req.buf.index -= header_len;
			conn->req.buf.total_index = conn->req.buf.index;
			conn->req.code = 1;
			
			conn->req.buf.total_length = 0;
			
			char* content_length = httpd_header_get(&conn->req, "Content-Length");
			char* transfer_encoding = httpd_header_get(&conn->req, "Transfer-Encoding");
			
			if(content_length) {
				conn->req.buf.total_length = (int)strtol(content_length, NULL, 10);
			}
			
			if(transfer_encoding) {
				if(strcmp(transfer_encoding, "chunked") == 0) {
					conn->req.encoding = HTTP_ENCODING_CHUNKED;
					parse_chunk(&conn->req);
				}
			}
			
			break;
		}
	}
	
	return ERR_OK;
}

// Fix the GCC 4.7.2's optimization bug
static void fix_gcc_bug(void* addr) {
	uint64_t v = (uint64_t)addr;
	if(v < 0x8000)
		printf("addr: %p\n", addr);
}

static Map* match(char* regex, char* str) {
	char* s = str;
	char* r = regex;
	
	Map* map = map_create(2, map_string_hash, map_string_equals, malloc, free);
	while(true) {
		char* r2 = strchr(r, ':');
		char* s2;
		if(r2) {
			if(strncmp(r, s, r2 - r) != 0)
				break;
			
			s += r2 - r;
			r = r2;
			
			r2 = strchr(r, '/');
			s2 = strchr(s, '/');
			
			int rlen;
			int slen;
			if(r2 == NULL && s2 == NULL) {
				rlen = strlen(r);
				slen = strlen(s);
			} else if(r2 != NULL && s2 != NULL) {
				rlen = r2 - r;
				slen = s2 - s;
			} else {
				break;
			}
			
			char* key = malloc(rlen);
			char* value = malloc(slen + 1);
			
			memcpy(key, r + 1, rlen - 1);
			key[rlen - 1] = '\0';
			memcpy(value, s, slen);
			value[slen] = '\0';
			
			map_put(map, key, value);
			
			r += rlen;
			s += slen;
			
			continue;
		}
		
		if(strcmp(r, s) == 0)
			return map;
		else
			break;
	}
	
	MapIterator iter;
	map_iterator_init(&iter, map);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_remove(&iter);
		
		free(entry->key);
		free(entry->data);
	}
	map_destroy(map);
	
	return NULL;
}

static err_t recv(void* arg, struct tcp_pcb* pcb, struct pbuf* pbuf, err_t err) {
	Conn* conn = arg;
	
	fix_gcc_bug(&pbuf);
	
	if(err == ERR_OK && pbuf != NULL) {
		if(conn->req.code == 0) {
			err = recv_header(conn, pcb, pbuf);
			if(err != ERR_OK)
				goto done;
		} else if(conn->req.buf.total_index < conn->req.buf.total_length || 
				conn->req.encoding == HTTP_ENCODING_CHUNKED) {
			err = recv_content(conn, pcb, pbuf);
			if(err != ERR_OK)
				goto done;
		} else {
			printf("TODO: Illegal HTTP... or re request in keep alive mode... %d %p \n", err, pbuf);
			goto done;
		}
		
		// Callback
		if(conn->req.code == 0)
			goto done;
		
		Map* cbs = NULL;
		switch(conn->method) {
			case METHOD_POST:
				cbs = conn->httpd->post;
				break;
			case METHOD_GET:
				cbs = conn->httpd->get;
				break;
			case METHOD_PUT:
				cbs = conn->httpd->put;
				break;
			case METHOD_DELETE:
				cbs = conn->httpd->delete;
				break;
			default:
				err = -1;
				goto done;
		}
		
		Callback* cb = map_get(cbs, conn->path);
		if(cb == NULL) {
			MapIterator iter;
			map_iterator_init(&iter, cbs);
			while(map_iterator_has_next(&iter)) {
				MapEntry* entry = map_iterator_next(&iter);
				char* ch = strchr(entry->key, ':');
				if(!ch)
					continue;
				
				Map* params = match(entry->key, conn->path);
				if(params) {
					MapIterator iter;
					map_iterator_init(&iter, params);
					while(map_iterator_has_next(&iter)) {
						MapEntry* entry = map_iterator_remove(&iter);
						
						map_put(conn->req.params, entry->key, entry->data);
					}
					map_destroy(params);
					
					cb = entry->data;
					break;
				}
			}
		}
		
		fix_gcc_bug(&cb);
		bool is_continue = true;
		if(cb != NULL) {
			is_continue = cb->callback(&conn->req, &conn->res);
		} else {
			conn->res.code = 404;
		}
		
		if(!is_continue) {
			printf("Close HTTP connection\n");
			close(conn, pcb, pbuf);
			return ERR_OK;
		}
	} else {
		close(conn, pcb, pbuf);
		return ERR_OK;
	}
	
done:
	if(pbuf) {
		tcp_recved(pcb, pbuf->tot_len);
		pbuf_free(pbuf);
	}
	
	return err;
}

static err_t sent(void* arg, struct tcp_pcb* tpcb, u16_t len) {
	Conn* conn = arg;
	
	ListIterator iter;
	list_iterator_init(&iter, conn->send_listeners);
	while(list_iterator_has_next(&iter)) {
		Listener* l = list_iterator_next(&iter);
		
		if(!l->listener(&conn->req, &conn->res, l->context)) {
			list_iterator_remove(&iter);
			free(l);
		}
	}
	
	return ERR_OK;
}

//static err_t poll(void* arg, struct tcp_pcb* pcb) {
//	printf("Poll %ld/%ld %d\n", pcb->keep_cnt_sent, pcb->keep_idle);
//	// TODO: Close when there is no recv/send some time
//	/*
//	printf("Poll close\n");
//	close(arg, pcb, NULL);
//	*/
//	
//	return ERR_OK;
//}

static void error(void *arg, err_t err) {
	Conn* conn = arg;
	close(conn, conn->pcb, NULL);
}

static err_t accept(void* arg, struct tcp_pcb* pcb, err_t err) {
	LWIP_UNUSED_ARG(err);
	
	HTTPD* httpd = arg;
	Conn* conn = malloc(sizeof(Conn));
	if(!conn)
		return ERR_OK;
	bzero(conn, sizeof(Conn));
	
	conn->httpd = httpd;
	
	conn->req.headers = map_create(8, map_string_hash, map_string_equals, malloc, free);
	conn->req.params = map_create(8, map_string_hash, map_string_equals, malloc, free);
	conn->req._conn = conn;
	
	conn->res.headers = map_create(8, map_string_hash, map_string_equals, malloc, free);
	conn->res.params = map_create(8, map_string_hash, map_string_equals, malloc, free);
	conn->res._conn = conn;
	
	conn->close_listeners = list_create(malloc, free);
	conn->send_listeners = list_create(malloc, free);
	
	conn->pcb = pcb;
	
	tcp_setprio(pcb, TCP_PRIO_MIN);
	tcp_recv(pcb, recv);
	tcp_sent(pcb, sent);
	tcp_err(pcb, error); // Don't care about error here
	//tcp_poll(pcb, poll, 4);
	tcp_arg(pcb, conn);
	
	return ERR_OK;
}

HTTPD* httpd_create(uint32_t address, int port) {
	struct ip_addr ip;
	IP4_ADDR(&ip, (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, (address >> 0) & 0xff);
	
	HTTPD* httpd = malloc(sizeof(HTTPD));
	if(!httpd)
		return NULL;
	
	httpd->post = map_create(4, map_string_hash, map_string_equals, malloc, free);
	if(!httpd->post)
		goto error;
	
	httpd->get = map_create(4,  map_string_hash, map_string_equals, malloc, free);
	if(!httpd->get)
		goto error;
	
	httpd->put = map_create(4,  map_string_hash, map_string_equals, malloc, free);
	if(!httpd->put)
		goto error;
	
	httpd->delete = map_create(4,  map_string_hash, map_string_equals, malloc, free);
	if(!httpd->delete)
		goto error;
	
	struct tcp_pcb* pcb = tcp_new();
	
	tcp_bind(pcb, &ip, port);
	pcb = tcp_listen(pcb);
	tcp_accept(pcb, accept);
	tcp_arg(pcb, httpd);

	return httpd;
	
error:
	if(httpd->post)
		map_destroy(httpd->post);
	
	if(httpd->get)
		map_destroy(httpd->get);
	
	if(httpd->put)
		map_destroy(httpd->put);
	
	if(httpd->delete)
		map_destroy(httpd->delete);
	
	free(httpd);
	
	return NULL;
}

static void callback_set(int method, HTTPD* httpd, char* path, HTTPCallback callback) {
	Callback* cb = malloc(sizeof(Callback));
	int path_len = strlen(path) + 1;
	cb->path = malloc(path_len);
	memcpy(cb->path, path, path_len);
	cb->callback = callback;
	
	Map* cbs = NULL;
	switch(method) {
		case METHOD_POST:
			cbs = httpd->post;
			break;
		case METHOD_GET:
			cbs = httpd->get;
			break;
		case METHOD_PUT:
			cbs = httpd->put;
			break;
		case METHOD_DELETE:
			cbs = httpd->delete;
			break;
	}
	
	map_put(cbs, path, cb);
}

void httpd_post(HTTPD* httpd, char* path, HTTPCallback callback) {
	callback_set(METHOD_POST, httpd, path, callback);
}

void httpd_get(HTTPD* httpd, char* path, HTTPCallback callback) {
	callback_set(METHOD_GET, httpd, path, callback);
}

void httpd_put(HTTPD* httpd, char* path, HTTPCallback callback) {
	callback_set(METHOD_PUT, httpd, path, callback);
}

void httpd_delete(HTTPD* httpd, char* path, HTTPCallback callback) {
	callback_set(METHOD_DELETE, httpd, path, callback);
}

void httpd_status_set(HTTPContext* ctx, int code, char* status) {
	if(ctx->status)
		free(ctx->status);
	
	if(!status)
		status = "Unknown";
	
	int len = strlen(status) + 1;
	
	ctx->code = code;
	ctx->status = malloc(len);
	memcpy(ctx->status, status, len);
}

void httpd_status_clear(HTTPContext* ctx) {
	if(ctx->status) {
		free(ctx->status);
		ctx->status = NULL;
	}
	
	ctx->code = 0;
}

void httpd_header_put(HTTPContext* ctx, char* key, char* value){
	char* key0 = map_get_key(ctx->headers, key);
	if(key0) {
		char* value0 = map_remove(ctx->headers, key);
		free(key0);
		free(value0);
	}
	
	int key_len = strlen(key) + 1;
	char* key2 = malloc(key_len);
	memcpy(key2, key, key_len);
	
	int value_len = strlen(value) + 1;
	char* value2 = malloc(value_len);
	memcpy(value2, value, value_len);
	
	map_put(ctx->headers, key2, value2);
}

char* httpd_header_get(HTTPContext* ctx, char* key){
	return map_get(ctx->headers, key);
}

bool httpd_header_remove(HTTPContext* ctx, char* key){
	char* key2 = map_get_key(ctx->headers, key);
	if(key2) {
		char* value = map_get(ctx->headers, key);
		free(key2);
		free(value);
		
		return true;
	} else {
		return false;
	}
}

void httpd_header_clear(HTTPContext* ctx) {
	MapIterator iter;
	map_iterator_init(&iter, ctx->headers);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_remove(&iter);
		
		free(entry->key);
		free(entry->data);
	}
}

void httpd_param_put(HTTPContext* ctx, char* key, char* value){
	char* key0 = map_get_key(ctx->params, key);
	if(key0) {
		char* value0 = map_remove(ctx->params, key);
		free(key0);
		free(value0);
	}
	
	int key_len = strlen(key) + 1;
	char* key2 = malloc(key_len);
	memcpy(key2, key, key_len);
	
	int value_len = strlen(value) + 1;
	char* value2 = malloc(value_len);
	memcpy(value2, value, value_len);
	
	map_put(ctx->params, key2, value2);
}

char* httpd_param_get(HTTPContext* ctx, char* key){
	return map_get(ctx->params, key);
}

bool httpd_param_remove(HTTPContext* ctx, char* key){
	char* key2 = map_get_key(ctx->params, key);
	if(key2) {
		char* value = map_get(ctx->params, key);
		free(key2);
		free(value);
		
		return true;
	} else {
		return false;
	}
}

void httpd_param_clear(HTTPContext* ctx) {
	MapIterator iter;
	map_iterator_init(&iter, ctx->params);
	while(map_iterator_has_next(&iter)) {
		MapEntry* entry = map_iterator_remove(&iter);
		
		free(entry->key);
		free(entry->data);
	}
}

uint64_t httpd_close_listener_add(HTTPContext* ctx, bool(*listener)(HTTPContext*,HTTPContext*,void*), void* context) {
	Listener* l = malloc(sizeof(Listener));
	l->listener = listener;
	l->context = context;
	
	Conn* conn = ctx->_conn;
	list_add(conn->close_listeners, l);
	
	return (uint64_t)l;
}

bool httpd_close_listener_remove(HTTPContext* ctx, uint64_t id) {
	Conn* conn = ctx->_conn;
	return list_remove_data(conn->close_listeners, (void*)id);
}

uint64_t httpd_send_listener_add(HTTPContext* ctx, bool(*listener)(HTTPContext*,HTTPContext*,void*), void* context) {
	Listener* l = malloc(sizeof(Listener));
	l->listener = listener;
	l->context = context;
	
	Conn* conn = ctx->_conn;
	list_add(conn->send_listeners, l);
	
	return (uint64_t)l;
}

bool httpd_send_listener_remove(HTTPContext* ctx, uint64_t id) {
	Conn* conn = ctx->_conn;
	return list_remove_data(conn->send_listeners, (void*)id);
}

void httpd_send(HTTPContext* res) {
	Conn* conn = res->_conn;
	HTTPContext* req = &conn->req;
	struct tcp_pcb* pcb = conn->pcb;
	
	// Response not ready
	if(res->code == 0) {	
		return;
	}
	
	// Send header
	if(res->code != 0 && res->code != -1) {
		// Add default headers
		/* TODO
		httpd_header_put(res, "Date", "Mon, 23 May 2005 22:38:34 GMT");
		httpd_header_put(res, "Last-Modified", "Wed, 08 Jan 2003 23:11:55 GMT");
		*/
		httpd_header_put(res, "Server", "HTTPD/1.0 (PacketNgin)");
		char* transfer_encoding = httpd_header_get(res, "Transfer-Encoding");
		if(transfer_encoding) {
			if(strcmp(transfer_encoding, "chunked") == 0) {
				res->encoding = HTTP_ENCODING_CHUNKED;
			}
		}
		
		if(res->encoding == HTTP_ENCODING_IDENTITY) {
			char content_length[16];
			sprintf(content_length, "%d", res->buf.total_length);
			httpd_header_put(res, "Content-Length", content_length);
		}
		
		if(!httpd_header_get(res, "Content-Type"))
			httpd_header_put(res, "Content-Type", "plain/text");
		
		// Calculate header length
		if(!res->status) {
			httpd_status_set(res, 200, "OK");
		}
		
		int header_status_len = strlen(res->status);
		int len = 13 + header_status_len  + 2;
		
		MapIterator iter;
		map_iterator_init(&iter, res->headers);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_next(&iter);
			len += strlen(entry->key) + 2;
			len += strlen(entry->data) + 2;
		}
		len += 2;
		
		// Check tcp buffer
		if(tcp_sndbuf(pcb) < len)
			return;
		
		// Make header buffer
		char* buf = malloc(len);
		if(!buf)
			return;
		
		// Make header status
		int idx = 0;
		memcpy(buf + idx, "HTTP/", 5); idx += 5;
		switch(conn->version) {
			case HTTP_1_0:
				buf[idx++] = '1'; buf[idx++] = '.'; buf[idx++] = '0'; buf[idx++] = ' ';
				break;
			default:
				buf[idx++] = '1'; buf[idx++] = '.'; buf[idx++] = '1'; buf[idx++] = ' ';
				break;
		}
		
		char header_code[4] = { ' ', };
		sprintf(header_code, "%d", res->code);
		memcpy(buf + idx, header_code, 3);
		idx += 3;
		buf[idx++] = ' ';
		
		memcpy(buf + idx, res->status, header_status_len);
		idx += header_status_len;
		buf[idx++] = '\r';
		buf[idx++] = '\n';
		
		httpd_status_clear(res);
		
		map_iterator_init(&iter, res->headers);
		while(map_iterator_has_next(&iter)) {
			MapEntry* entry = map_iterator_remove(&iter);
			
			int key_len = strlen(entry->key);
			memcpy(buf + idx, entry->key, key_len);
			free(entry->key);
			idx += key_len;
			buf[idx++] = ':'; buf[idx++] = ' ';
			
			int value_len = strlen(entry->data);
			memcpy(buf + idx, entry->data, value_len);
			free(entry->data);
			idx += value_len;
			buf[idx++] = '\r'; buf[idx++] = '\n';
		}
		buf[idx++] = '\r'; buf[idx++] = '\n';
		
		err_t err = tcp_write(pcb, buf, len, TCP_WRITE_FLAG_COPY);
		if(err != ERR_OK) {
			printf("Error code: %d\n", err);
		}
		
		free(buf);
		
		res->code = -1;
	}
	
	// Send content
	if(res->buf.index > 0) {
		int len = tcp_sndbuf(pcb);
		if(res->buf.index < len)
			len = res->buf.index;
		
		err_t err = tcp_write(pcb, res->buf.buf, len, TCP_WRITE_FLAG_COPY);
		if(err != ERR_OK) {
			printf("Error code: %d\n", err);
		}
		
		memmove(res->buf.buf, res->buf.buf + len, res->buf.index - len);
		res->buf.index -= len;
		res->buf.total_index += len;
	} 
	
	// End of response
	if(req->buf.total_index >= req->buf.total_length && res->buf.total_index >= res->buf.total_length) {
		if(res->encoding == HTTP_ENCODING_IDENTITY) {
			req->code = 0;
			req->encoding = 0;
			req->buf.index = 0;
			req->buf.total_index = 0;
			req->buf.total_length = 0;
			httpd_header_clear(req);
			
			res->code = 0;
			res->encoding = 0;
			res->buf.index = 0;
			res->buf.total_index = 0;
			res->buf.total_length = 0;
			httpd_header_clear(res);
		}
		/*
		if(conn->version == HTTP_1_0) {
			is_continue = false;
		} else {
			char* header_conn = httpd_header_get(&conn->res, "Connection");
			if(header_conn != NULL && strcmp(header_conn, "close") == 0) {
				is_continue = false;
			}
		}
		*/
	}
}

void httpd_close(HTTPContext* ctx) {
	Conn* conn = ctx->_conn;
	close(conn, conn->pcb, NULL);
}

