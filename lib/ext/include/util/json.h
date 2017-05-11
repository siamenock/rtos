#ifndef __UTIL_JSON_H__
#define __UTIL_JSON_H__

#include <stdbool.h>
#include <util/map.h>

/**
 * @file
 * JSON (JavsCript Object Notation) parser
 */

#define JSON_DUMP	0	///< To compile JSON dump function

#define JSON_PRIMITIVE	0	///< The object is JSON primitive value such as a number
#define JSON_OBJECT	1	///< The object is key-value object
#define JSON_ARRAY	2	///< The object is array
#define JSON_STRING	3	///< The object is a string

/**
 * JSONType is abstract structure of JSONData, Attr, Array and Object
 */
typedef struct {
	int		type;	///< Type
} JSONType;

/**
 * JSONData stores primitive or string data
 */
typedef struct {
	int		type;	///< Type
	int		size;	///< Array size or string length
	void*		data;	///< Array itself or char string
} JSONData;

/**
 * JSONAttr is key-value pair
 */
typedef struct {
	char*		name;	///< Key name
	int		type;	///< Type
	void*		data;	///< Value
} JSONAttr;

/**
 * JSONArray
 */
typedef struct {
	int		type;	///< Type
	JSONData*	array;	///< Array of JSONData
	int		size;	///< Array size
} JSONArray;

/**
 * JSONObject is key-value object
 */
typedef struct {
	int		type;	///< Type
	JSONAttr*	attrs;	///< Attributes (key-value pair)
	int		size;	///< Number of attributes
} JSONObject;

/**
 * Parse text to JSON.
 *
 * @param text text to parse
 * @param len text length
 * @return any JSON object (JSONData, Array or Object)
 */
JSONType* json_parse(char* text, size_t len);

/**
 * Free prased JSON.
 *
 * @param json JSON object
 */
void json_free(JSONType* json);

/**
 * Find attribute from JSONObject.
 *
 * @param obj JSONObject
 * @param name attribute name
 * @return JSONAttr or NULL if not found
 */
JSONAttr* json_get(JSONObject* obj, char* name);

#if JSON_DUMP
/**
 * Dump JSON to standard output
 *
 * @param json any JSON object
 */
void json_dump(JSONType* json);
#endif

#endif /* __UTIL_JSON_H__ */
