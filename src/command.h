#ifndef __COMMAND_H__
#define __COMMAND_H__

/** 
 * Process input command from file descriptor.
 *
 * @param file descriptor of input source. (e.g stdio)
 */
void command_process(int fd);

#endif /* __COMMAND_H__ */
