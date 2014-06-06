#include <string.h>
#include <util/fifo.h>
#include <util/ring.h>
#include <malloc.h>
#include "../stdio.h"
#include "../shared.h"
#include "../gmalloc.h"
#include "../mp.h"
#include "stdout.h"

typedef struct {
	char*		buffer;
	size_t*		head;
	size_t*		tail;
	size_t		size;
} Buffer;

static Buffer buffers[2];

static int init(void* device, void* data) {
	extern char __stdout[];
	extern size_t __stdout_head;
	extern size_t __stdout_tail;
	extern size_t __stdout_size;

	extern char __stderr[];
	extern size_t __stderr_head;
	extern size_t __stderr_tail;
	extern size_t __stderr_size;
	
	if(strcmp(data, "stdout") == 0) {
		buffers[0].buffer = __stdout;
		buffers[0].head = &__stdout_head;
		buffers[0].tail = &__stdout_tail;
		buffers[0].size = __stdout_size;
		
		return 0;
	} else if(strcmp(data, "stderr") == 0) {
		buffers[1].buffer = __stderr;
		buffers[1].head = &__stderr_head;
		buffers[1].tail = &__stderr_tail;
		buffers[1].size = __stderr_size;
		
		return 1;
	}
	
	return -1;
}

static void destroy(int id) {
}

static int write(int id, const char* buf, int len) {
	Buffer* buffer = &buffers[id];
	return ring_write(buffer->buffer, *buffer->head, buffer->tail, buffer->size, buf, len);
}

DeviceType stdout_type = DEVICE_TYPE_CHAR_OUT;

CharOut stdout_driver = {
	.init = init,
	.destroy = destroy,
	.write = write,
};
