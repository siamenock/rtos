#ifndef __CMD_H__
#define __CMD_H__

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

extern Command commands[];
extern char cmd_result[];

extern void cmd_init();
extern int cmd_help(int argc, char** argv, void(*callback)(char* result, int exit_status));
extern int cmd_exec(char* line, void(*callback)(char* result, int exit_status));
extern void cmd_update_var(char* result, int exit_status);

#endif /* __CMD_H__ */
