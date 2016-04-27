#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <util/json.h>
#include <jsmn.h>

#if JSON_DUMP
#include <stdio.h>
#endif

#define JSMN_TOKEN_COUNT	2048

#if JSON_DUMP
static int depth = 0;

static void tap() {
	for(int i = 0; i < depth; i++)
		printf("\t");
}

static void print(char* text, int start, int end) {
	for(int i = start; i < end; i++)
		printf("%c", text[i]);
}
#endif

static JSONObject* parse_object(char* text, jsmntok_t* tokens, int* index);
static JSONArray* parse_array(char* text, jsmntok_t* tokens, int* index);
static void* parse(char* text, jsmntok_t* tokens, int* index);

static int mcount = 0;
static int fcount = 0;

static JSONObject* parse_object(char* text, jsmntok_t* tokens, int* index) {
	if(tokens[*index].type != JSON_OBJECT)
		return NULL;
	
	int count = tokens[*index].size / 2;
	
	JSONObject* obj = malloc(sizeof(JSONObject));
	obj->type = JSON_OBJECT;
	mcount++;
	obj->attrs = malloc(sizeof(JSONAttr) * count);
	mcount++;
	obj->size = count;
	
	#if JSON_DUMP
	printf("{\n");
	depth++;
	#endif
	
	for(int i = 0; i < count; i++) {
		JSONAttr* attr = obj->attrs + i;
		(*index)++;
		#if JSON_DUMP
		tap(); print(text, tokens[*index].start, tokens[*index].end); printf(": ");
		#endif
		
		attr->name = text + tokens[*index].start;
		text[tokens[*index].end] = '\0';
		(*index)++;
		attr->type = tokens[*index].type;
		attr->data = parse(text, tokens, index);
		#if JSON_DUMP
		if(i + 1 < count)
			printf(",\n");
		else
			printf("\n");
		#endif
	}
	
	#if JSON_DUMP
	depth--;
	tap(); printf("}");
	#endif
	
	return obj;
}

static JSONArray* parse_array(char* text, jsmntok_t* tokens, int* index) {
	if(tokens[*index].type != JSON_ARRAY)
		return NULL;
	
	int count = tokens[*index].size;
	
	JSONArray* array = malloc(sizeof(JSONArray));
	array->type = JSON_ARRAY;
	mcount++;
	array->array = malloc(sizeof(JSONData) * count);
	mcount++;
	array->size = count;
	
	#if JSON_DUMP
	printf("[\n");
	depth++;
	#endif
	
	for(int i = 0; i < count; i++) {
		(*index)++;
		#if JSON_DUMP
		tap(); 
		#endif
		array->array[i].type = tokens[*index].type;
		array->array[i].size = tokens[*index].size;
		array->array[i].data = parse(text, tokens, index); 
		
		#if JSON_DUMP
		if(i + 1 < count)
			printf(",\n");
		else
			printf("\n");
		#endif
	}
	
	#if JSON_DUMP
	depth--;
	tap(); printf("]");
	#endif
	
	return array;
}

static void* parse(char* text, jsmntok_t* tokens, int* index) {
	switch(tokens[*index].type) {
		case JSMN_PRIMITIVE:
			#if JSON_DUMP
			print(text, tokens[*index].start, tokens[*index].end);
			#endif
			text[tokens[*index].end] = '\0';
			return text + tokens[*index].start;
		case JSMN_OBJECT:
			return parse_object(text, tokens, index);
		case JSMN_ARRAY:
			return parse_array(text, tokens, index);
		case JSMN_STRING:
			#if JSON_DUMP
			printf("\""); print(text, tokens[*index].start, tokens[*index].end); printf("\"");
			#endif
			text[tokens[*index].end] = '\0';
			return text + tokens[*index].start;
	}
	return NULL;
}

JSONType* json_parse(char* text, size_t len) {
	static jsmntok_t tokens[JSMN_TOKEN_COUNT];
	
	jsmn_parser parser;
	jsmn_init(&parser);
	
	int count = jsmn_parse(&parser, text, len, tokens, JSMN_TOKEN_COUNT);
	if(count < 0) {
		errno = count;
		return NULL;
	}
	
	int i = 0;
	void* data = parse(text, tokens, &i);
	switch(tokens[0].type) {
		case JSON_PRIMITIVE:
		case JSON_STRING:
		{
			JSONData* json = malloc(sizeof(JSONData));
			json->type = tokens[0].type;
			json->data = data;
			json->size = 0;
			mcount++;
			return (JSONType*)json;
		}
		default:
			return data;
	}
}

#if JSON_DUMP
static void dump(int type, void* data);

static void dump_object(JSONObject* obj) {
	printf("{\n");
	depth++;
	for(int i = 0; i < obj->size; i++) {
		JSONAttr* attr = obj->attrs + i;
		tap(); printf("%s: ", attr->name);
		dump(attr->type, attr->data);
		if(i + 1 < obj->size)
			printf(",\n");
		else
			printf("\n");
	}
	depth--;
	tap(); printf("}");
}

static void dump_array(JSONArray* array) {
	printf("[\n");
	depth++;
	for(int i = 0; i < array->size; i++) {
		JSONData* data = &array->array[i];
		tap(); dump(data->type, data->data);
		if(i + 1 < array->size)
			printf(",\n");
		else
			printf("\n");
	}
	depth--;
	tap(); printf(" ]");
}

static void dump(int type, void* data) {
	switch(type) {
		case JSON_PRIMITIVE:
			printf("%s", data);
			break;
		case JSON_OBJECT:
			dump_object(data);
			break;
		case JSON_ARRAY:
			dump_array(data);
			break;
		case JSON_STRING:
			printf("\"%s\"", data);
			break;
	}
}

void json_dump(JSONType* json) {
	switch(json->type) {
		case JSON_PRIMITIVE:
		case JSON_STRING:
			dump(json->type, ((JSONData*)json)->data);
			break;
		case JSON_OBJECT:
			dump_object((JSONObject*)json);
			break;
		case JSON_ARRAY:
			dump_array((JSONArray*)json);
			break;
	}
}
#endif

static void _free(int type, void* data) {
	if(data == NULL)
		return;
	
	JSONObject* obj;
	JSONArray* array;
	
	switch(type) {
		case JSON_PRIMITIVE:
			break;
		case JSON_OBJECT:
			obj = data;
			for(int i = 0; i < obj->size; i++) {
				_free(obj->attrs[i].type, obj->attrs[i].data);
			}
			free(obj->attrs);
			fcount++;
			free(obj);
			fcount++;
			break;
		case JSON_ARRAY:
			array = data;
			for(int i = 0; i < array->size; i++) {
				_free(array->array[i].type, array->array[i].data);
			}
			free(array->array);
			fcount++;
			free(array);
			fcount++;
			break;
		case JSON_STRING:
			break;
	}
}

void json_free(JSONType* json){
	switch(json->type) {
		case JSON_PRIMITIVE:
		case JSON_STRING:
			free(json);
			fcount++;
			break;
		case JSON_OBJECT:
			_free(json->type, (JSONObject*)json);
			break;
		case JSON_ARRAY:
			_free(json->type, (JSONArray*)json);
			break;
	}
}

JSONAttr* json_get(JSONObject* obj, char* name) {
	for(int i = 0; i < obj->size; i++) {
		if(strcmp(obj->attrs[i].name, name) == 0)
			return &obj->attrs[i];
	}
	
	return NULL;
}

#if JSON_DUMP
void main() {
	char text[4096];
	int ch = fgetc(stdin);
	int i = 0;
	while(ch > 0) {
		text[i++] = ch;
		ch = fgetc(stdin);
	}
	int len = i;
	
	/*
	for(int i = 0; i < count; i++) {
		printf("[%d + %d] ", i, tokens[i].size);
		switch(tokens[i].type) {
			case JSMN_OBJECT:
				printf("object "); print(text, tokens[i].start, tokens[i].end); printf(" %d\n", tokens[i].size);
				break;
			case JSMN_ARRAY:
				printf("array "); print(text, tokens[i].start, tokens[i].end); printf(" %d\n", tokens[i].size);
				break;
			case JSMN_STRING:
				printf("string "); printf("\""); print(text, tokens[i].start, tokens[i].end); printf("\"");
				printf("\n");
				break;
			case JSMN_PRIMITIVE:
				printf("primitive "); print(text, tokens[i].start, tokens[i].end);
				printf("\n");
				break;
		}
	}
	
	printf("\n");
	printf("count = %d\n", count);
	*/
	JSONType* json = json_parse(text, len);
	printf("\n");
	
	json_dump(json);
	json_free(json);
	printf("\nmalloc=%d/free=%d\n", mcount, fcount);
}
#endif
