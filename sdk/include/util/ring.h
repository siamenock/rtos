#ifndef __UTIL_RING_H__
#define __UTIL_RING_H__

#include <unistd.h>

ssize_t ring_write(char* buf, size_t head, volatile size_t* tail, size_t size, char* data, size_t len);
ssize_t ring_read(char* buf, volatile size_t *head, size_t tail, size_t size, char* data, size_t len);
size_t ring_readable(size_t head, size_t tail, size_t size);
size_t ring_writable(size_t head, size_t tail, size_t size);

#endif /* __UTIL_RING_H__ */
