#ifndef __SLOWPATH_H__
#define __SLOWPATH_H__


#include <vnic.h>

int slowpath_up(VNIC* vnic);
int slowpath_create(VNIC* vnic);
int slowpath_destroy(VNIC* vnic);
bool slowpath_init();
int slowpath_interface_add(VNIC* vnic);
int slowpath_interface_remove(VNIC* vnic);

#endif /*__SLOWPATH_H__*/