#ifndef __NETOB_H__
#define __NETOB_H__

/**
 * @file
 * Network Interface Observer (netob)
 *
 * This module provides the following functions using netlink
 * - Scan all of existing linux network interfaces
 * - Event listening
 *   - Netowrk interface up/down event
 *   - Link level configuration modification event
 *   - IP configuration modification event
 *
 * Events are delivered as shown in the following sequence:
 * linux kernel --(netlink connection)--> netob --(shared memory)--> packetngin
 */

/**
 * Initialize module and start observing
 *
 * @return zero for success, nonzero for failure
 */
int netob_init();

/**
 * Finalize module
 *
 * @return zero for success, nonzero for failure
 */
int netob_fini();

#endif // __NETOB_H__
