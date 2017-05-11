#ifndef __THREAD_H__
#define __THREAD_H__

/**
 * @file
 * Thread management.
 */

/**
 * Get thread ID. Thread ID must be allocated from 0 sequencially. 
 * So thread ID 0 means the first CPU Core of virtual machine.
 *
 * @return thread ID
 */
int thread_id();

/**
 * Get total thread count of virtual machine.
 *
 * @return number of threads
 */
int thread_count();

/**
 * Thread bariior. Wait every threads reach the point of the code.
 */
void thread_barrior();

#endif /* __THREAD_H__ */
