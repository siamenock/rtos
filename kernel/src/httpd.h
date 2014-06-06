#ifndef __HTTPD_H__
#define __HTTPD_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct _HTTPD HTTPD;
typedef struct _Map Map;

#define HTTP_BUFFER_SIZE	8192

#define HTTP_ENCODING_IDENTITY	0
#define HTTP_ENCODING_CHUNKED	1

typedef struct {
	uint8_t	buf[HTTP_BUFFER_SIZE];
	int	index;
	
	int	total_index;
	int	total_length;
} HTTPBuffer;

typedef struct {
	int		code;
	char*		status;
	int		encoding;
	Map*		headers;
	Map*		params;
	HTTPBuffer	buf;
	
	void*		_conn;
} HTTPContext;

typedef bool(*HTTPCallback)(HTTPContext* req, HTTPContext* res);

HTTPD* httpd_create(uint32_t address, int port);
void httpd_post(HTTPD* httpd, char* path, HTTPCallback callback);
void httpd_get(HTTPD* httpd, char* path, HTTPCallback callback);
void httpd_put(HTTPD* httpd, char* path, HTTPCallback callback);
void httpd_delete(HTTPD* httpd, char* path, HTTPCallback callback);

void httpd_status_set(HTTPContext* ctx, int code, char* status);
void httpd_status_clear(HTTPContext* ctx);

void httpd_header_put(HTTPContext* ctx, char* key, char* value);
char* httpd_header_get(HTTPContext* ctx, char* key);
bool httpd_header_remove(HTTPContext* ctx, char* key);
void httpd_header_clear(HTTPContext* ctx);

void httpd_param_put(HTTPContext* ctx, char* key, char* value);
char* httpd_param_get(HTTPContext* ctx, char* key);
bool httpd_param_remove(HTTPContext* ctx, char* key);
void httpd_param_clear(HTTPContext* ctx);

uint64_t httpd_close_listener_add(HTTPContext* ctx, bool(*listener)(HTTPContext*,HTTPContext*,void*), void* context);
bool httpd_close_listener_remove(HTTPContext* ctx, uint64_t id);

uint64_t httpd_send_listener_add(HTTPContext* ctx, bool(*listener)(HTTPContext*,HTTPContext*,void*), void* context);
bool httpd_send_listener_remove(HTTPContext* ctx, uint64_t id);

void httpd_send(HTTPContext* res);
void httpd_close(HTTPContext* ctx);

#endif /* __HTTPD_H__ */
