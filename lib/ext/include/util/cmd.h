#ifndef __CMD_H__
#define __CMD_H__

#include <util/fifo.h>

#define CMD_MAX_ARGC 256
#define CMD_RESULT_SIZE 4096
#define CMD_STATUS_WRONG_NUMBER -1000
#define CMD_STATUS_NOT_FOUND -1001
#define CMD_VARIABLE_NOT_FOUND -2000

typedef struct {
	char* name;
	char* desc;
	char* args;
	int (*func)(int argc, char** argv, void(*callback)(char* result, int exit_status));
} Command;

/*
 * Command history which save previously executed commands
 */
typedef struct {
	FIFO*	histories;		///< History strings (internal use only)
	int	index;			///< Currnet history index

	void	(*reset)();		///< Reset history index to 0
	bool	(*using)();		///< Flag which indicates history being using or not
	int	(*save)(char* line);	///< Save executed command string line
	int	(*count)();		///< Get the number of histories
	char*	(*get_past)();		///< Get previus executed command
	char*	(*get_current)();	///< Get current executed command
	char*	(*get_later)();		///< Get later executed command
} CommandHistory;

extern Command commands[];
extern char cmd_result[];

extern void cmd_init();
extern int cmd_help(int argc, char** argv, void(*callback)(char* result, int exit_status));
extern int cmd_exec(char* line, void(*callback)(char* result, int exit_status));
extern void cmd_update_var(char* result, int exit_status);

extern CommandHistory cmd_history;

#endif /* __CMD_H__ */
