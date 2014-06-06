#ifndef __UTIL_JSON_H__
#define __UTIL_JSON_H__

#include <stdbool.h>
#include <util/map.h>

#define JSON_DUMP	0

#define JSON_PRIMITIVE	0
#define JSON_OBJECT	1
#define JSON_ARRAY	2
#define JSON_STRING	3

typedef struct {
	int		type;
} JSONType;

typedef struct {
	int		type;
	int		size;
	void*		data;
} JSONData;

typedef struct {
	char*		name;
	int		type;
	void*		data;
} JSONAttr;

typedef struct {
	int		type;
	JSONData*	array;
	int		size;
} JSONArray;

typedef struct {
	int		type;
	JSONAttr*	attrs;
	int		size;
} JSONObject;

JSONType* json_parse(char* text, size_t len);
void json_free(JSONType* json);
JSONAttr* json_get(JSONObject* obj, char* attr);

#if JSON_DUMP
void json_dump(JSONType* json);
#endif

#endif /* __UTIL_JSON_H__ */
