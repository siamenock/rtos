#include <stddef.h>
#include "stdin.h"

static int init(void* device, void* data) {
	return 0;
}

static void destroy(int id) {
}

DeviceType stdin_type = DEVICE_TYPE_CHAR_IN;

CharIn stdin_driver = {
	.init = init,
	.destroy = destroy,
};
