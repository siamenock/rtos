#ifndef __READLINE_H__
#define __READLINE_H__

#define READLINE_MAX_LINE_SIZE	4096

/**
 * @file
 * Read a line from standard input asynchronously.
 */

/**
 * Read line from standard input asynchronously.
 *
 * @return the pointer of line (which ends width \0) or NULL if there is no line
 */
char* readline();

#endif /* __READLINE_H__ */
