#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>
#include <stdint.h>

bool vnic_add(int idx, uint64_t addr);
bool vnic_remove(int idx, uint64_t addr);
bool vnic_output(int idx, uint64_t addr);

#endif/* __COMMAND_H__ */
