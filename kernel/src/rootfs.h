#ifndef __ROOTFS_H__
#define __ROOTFS_H__

#include <stdint.h>
#include <stdbool.h>

void* rootfs_addr();
uint32_t rootfs_size();

void* rootfs_file(const char* name, uint32_t* size);
bool rootfs_rewind();
void* rootfs_next(char* name, uint32_t* size);

#endif /* __ROOTFS_H__ */
