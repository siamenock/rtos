#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <stdbool.h>

typedef enum {
	OPTION_TYPE_INT,	// 4 Byte
	OPTION_TYPE_LONG,	// 8 Byte
	OPTION_TYPE_STRING,
} OptionType;

typedef struct _Option {
	char* name;		///< Option name
	char flag;		///< Short flag for option
	char* desc;		///< Option description
	bool required;		///< Wheter option is mandatory or not
	OptionType type;	///< Option type
} Option;

typedef struct _Command {
	char* name;
	char* desc;
	Option options[];
} Command;

void help();

#endif /* __COMMAND_H__ */
