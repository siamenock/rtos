#ifndef __MULTIBOOT_TAG_H__
#define __MULTIBOOT_TAG_H__

#include <stdint.h>
#include "multiboot2.h"
void* multiboot_tag_get(uint32_t multiboot_tag_type);

#endif
