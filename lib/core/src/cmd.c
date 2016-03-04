#include <stdbool.h>
#include <_string.h>
#include <malloc.h>
#include <util/cmd.h>
#include <util/map.h>

static Map* variables = NULL;
static char* variable = NULL;
char cmd_result[CMD_RESULT_SIZE];

int cmd_help(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
        int command_len = 0;
	if(argc == 1) {
		for(int i = 0; commands[i].name != NULL; i++) {
			int len = strlen(commands[i].name);
			command_len = len > command_len ? len : command_len;
		}

		for(int i = 0; commands[i].name != NULL; i++) {
			printf("%s", commands[i].name);
			int len = strlen(commands[i].name);
			len = command_len - len + 2;
			for(int j = 0; j < len; j++)
			       putchar(' ');
			if(commands[i].args != NULL) {
				printf("%s  %s\n", commands[i].desc, commands[i].args);
			} else
				printf("%s\n", commands[i].desc);
		}
	} else if(argc == 2) {
		for(int i = 0; commands[i].name != NULL; i++) {
			if(!strcmp(commands[i].name, argv[1])) {
				printf("%s", commands[i].name);
				int len = strlen(commands[i].name);
				len = len + 2;
				for(int j = 0; j < len; j++)
				       putchar(' ');
				if(commands[i].args != NULL) {
					printf("%s  %s\n", commands[i].desc, commands[i].args);
				} else
					printf("%s\n", commands[i].desc);

				goto end;
			}
		}
		printf("no help topics match '%s'\n", argv[1]);
		if(callback != NULL)
			callback("false", 0);

		return 0;
	}

end:
	if(callback != NULL)
		callback("true", 0);

        return 0;
}

static int cmd_parse_line(char* line, char** argv) {
	int argc = 0;
	bool is_start = true;
	char quotation = 0;
	int str_len =  strlen(line);
	char* start = line;
	for(int i = 0;i < (str_len + 1); i++) {
		if(quotation != 0) {
			if(line[i] == quotation) {
				quotation = 0;
				line[i] = '\0';
				argv[argc++] = start;
				is_start = true;
			}
		} else {
			switch(line[i]) {
				case '\'':
				case '"':
					quotation = line[i];
					line[i] = '\0';
					start = &line[i + 1];
					break;
				case ' ':
				case '\0':
					if(is_start == false) {
						line[i] = '\0';
						argv[argc++] = start;
						is_start = true;
					}
					break;
				default:
					if(is_start == true) {
						start = &line[i];
						is_start = false;
					}
					break;
			}
		}
	}

	return argc;
}

static void cmd_parse_var(int* argc, char** argv) {
	if(*argc >= 2) {
		if((argv[0][0] == '$') && (argv[1][0] == '=')) {
			if(variable) {
				free(variable);
			}
			variable = strdup(argv[0]);
			memmove(&argv[0], &argv[2], sizeof(char*) * (*argc - 2));
			*argc -= 2;
		}
	}
}

static bool cmd_parse_arg(int argc, char** argv) {
	for(int i = 0; i < argc; i++) {
		if(argv[i][0] == '$') {
			if(!map_contains(variables, argv[i])) {
				return false;
			} else {
				argv[i] = map_get(variables, argv[i]);
			}
		}
	}
	return true;
}

static Command* cmd_get(int argc, char** argv) {
	if(argc == 0)
		return NULL;

        for(int i = 0; commands[i].name != NULL; i++) {
                if(strcmp(argv[0], commands[i].name) == 0) {
                        return &commands[i];
                }
        }
        if(argc > 0)
                printf("%s : command not found\n", argv[0]);

        return NULL;
}

void cmd_update_var(char* result, int exit_status) {
	char buf[16];
	sprintf(buf, "%d", exit_status);
	
	free(map_get(variables, "$?"));
	map_update(variables, "$?", strdup(buf));

	if(exit_status == 0) {
		if(variable) {
			if(map_contains(variables, variable)) {
				free(map_get(variables, variable));
				if(strlen(result) > 0)
					map_update(variables, variable, strdup(result));
				else
					map_update(variables, variable, strdup("(nil)"));		
			} else {
				if(strlen(result) > 0)
					map_put(variables, strdup(variable), strdup(result));
				else
					map_put(variables, strdup(variable), strdup("(nil)"));
			}
		}
	}

	if(variable) {
		free(variable);
		variable = NULL;
	}
}

void cmd_init(void) {
	variables = map_create(16, map_string_hash, map_string_equals, NULL);
	map_put(variables, strdup("$?"), strdup("(nil)"));
	map_put(variables, strdup("$nil"), strdup("(nil)"));
}

//1. check argument
//2. get variable
//3. 
int cmd_exec(char* line, void(*callback)(char* result, int exit_status)) {
	int argc;
	char* argv[CMD_MAX_ARGC];
	
	argc = cmd_parse_line(line, argv);
	if(argc == 0 || argv[0][0] == '#')
		return 0;

	cmd_parse_var(&argc, argv);

	if(cmd_parse_arg(argc, argv) == false)
		return CMD_VARIABLE_NOT_FOUND;

	Command* cmd = cmd_get(argc, argv);
	int exit_status = CMD_STATUS_NOT_FOUND;
	if(cmd) {
		cmd_result[0] = '\0';
		exit_status = cmd->func(argc, argv, callback);
	}

	return exit_status;
}
