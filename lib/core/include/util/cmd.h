#ifndef __CMD_H__
#define __CMD_H__

#define CMD_MAX_ARGC 256
#define CMD_RESULT_SIZE 4096
#define CMD_ASYNC_FUNC INT_MIN //limits.h

typedef struct {
	char* name;
	char* desc;
	char* args;
	int (*func)(int argc, char** argv);
} Command;

extern Command commands[];
extern char cmd_result[];

extern void cmd_async_result(char* result, int exit_status);
extern void cmd_init(void);
extern int cmd_help(int argc, char** argv);
extern int cmd_exec(char* line);

#endif /* __CMD_H__ */
