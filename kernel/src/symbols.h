#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

#include <stdbool.h>

#define SYMBOL_TYPE_NONE	0
#define SYMBOL_TYPE_FUNC	1
#define SYMBOL_TYPE_DATA	2

typedef struct _Symbol {
	char*	name;
	int	type;
	void*	address;
} Symbol;

void symbols_init();
Symbol* symbols_get(char* name);

#endif /* __SYMBOLS_H__ */
