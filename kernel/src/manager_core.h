#ifndef __MANAGER_CORE_H__
#define __MANAGER_CORE_H__

#include <stdint.h>
#include <stdbool.h>

#include <control/rpc.h>

/**
 * Core metadata
 */
typedef struct _ManagerCore {
	uint16_t	port; ///< tcp port
	void*		data; ///< `fd` on linux stack, `pcb` on lwip stack
	List*		clients;
} ManagerCore;

/**
 * Initialize manager core
 *
 * @return zero for success, nonzero for failure
 */
int manager_core_init(int (*accept)(RPC* rpc));

/**
 * Open manager core server
 *
 * @param port server port number (tcp port)
 * @return ManagerCore for success, NULL for failure
 */
ManagerCore* manager_core_server_open(uint16_t port);

/**
 * Close manager core server
 *
 * @param manager_core core data
 * @return true for success, false for failure
 */
bool manager_core_server_close(ManagerCore* manager_core);

#endif /*__MANAGER_CORE_H__*/
