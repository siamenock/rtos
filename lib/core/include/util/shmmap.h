#ifndef __UTIL_MAP_H__
#define __UTIL_MAP_H__

#include <stdint.h>
#include "list.h"

/**
 * @file
 * Hash Shmmap data structure
 */

/**
 * Hash shmmap entry data structure (internal use only)
 */
typedef struct _ShmmapEntry {
	void*	key;			///< Key
	void*	data;			///< Value
} ShmmapEntry;

/**
 * Hash Shmmap data structure
 */
typedef struct _Shmmap {
	List**		table;		///< Shmmap table (internal use only)
	size_t		threshold;	///< Threshold to extend the table (internal use only)
	size_t		capacity;	///< Current capacity (internal use only)
	size_t		size;		///< Number of elements (internal use only)
	
	uint8_t		hash_type;	///< hashing function
	uint8_t		equals_type;	///< comparing function
	
	void*		pool;		///< Memory pool (internal use only)
} __attribute__((packed)) Shmmap;

/**
 * Create a HashShmmap.
 *
 * @param initial_capacity respected maximum number of elements
 * @param hash key hashing function, if hash is NULL shmmap_uint64_hash will be used
 * @param equals key comparing function, if equals is NULL shmmap_uint64_equals will be used
 * @param pool memory pool to use, if NULL local memory area will be used
 */
Shmmap* shmmap_create(size_t initial_capacity, uint8_t hash_type, uint8_t equals_type, void* pool);

/**
 * Destroy the HashShmmap.
 *
 * @param shmmap HashShmmap
 */
void shmmap_destroy(Shmmap* shmmap);

/**
 * Check the HashShmmap is empty or not.
 *
 * @param shmmap HashShmmap
 * @return true if the HashShmmap is empty
 */
bool shmmap_is_empty(Shmmap* shmmap);

/**
 * Put an element to the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @param key key of element
 * @param data data of element
 * @return true if the element is putted, false if there is an element with same key or memory is full
 */
bool shmmap_put(Shmmap* shmmap, void* key, void* data);

/**
 * Update an element with new key and data.
 *
 * @param shmmap HashShmmap
 * @param key new key of the element
 * @param data new data of the element
 * @return true if the element is updated, false if there is no such element
 */
bool shmmap_update(Shmmap* shmmap, void* key, void* data);

/**
 * Get an element data from the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @param key key of the element
 * @return the element's data or NULL if there is no such element
 */
void* shmmap_get(Shmmap* shmmap, void* key);

/**
 * Get an element key from the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @param key key of the element
 * @return the element's key or NULL if there is no such element
 */
void* shmmap_get_key(Shmmap* shmmap, void* key);

/**
 * Check there is an element.
 *
 * @param shmmap HashShmmap
 * @param key key of the element
 * @return true if there is an element with the key
 */
bool shmmap_contains(Shmmap* shmmap, void* key);

/**
 * Remove an element from the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @param key key of the element
 * @return removed element or NULL if nothing is removed
 */
void* shmmap_remove(Shmmap* shmmap, void* key);

/**
 * Get the current capacity of the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @return capacity of the HashShmmap
 */
size_t shmmap_capacity(Shmmap* shmmap);

/**
 * Get the number of elements of the HashShmmap.
 *
 * @param shmmap HashShmmap
 * @return number of elements
 */
size_t shmmap_size(Shmmap* shmmap);

/**
 * Iterator of a HashShmmap
 */
typedef struct _ShmmapIterator {
	Shmmap*		shmmap;		///< HashShmmap (internal use only)
	size_t		index;		///< Current index of table (internal use only)
	size_t		list_index;	///< Current index of list (internal use only)
	ShmmapEntry	entry;		///< Temporary ShmmapEntry
} ShmmapIterator;

/**
 * Initialize the iterator.
 *
 * @param iter the iterator
 * @param shmmap HashShmmap
 */
void shmmap_iterator_init(ShmmapIterator* iter, Shmmap* shmmap);

/**
 * Check there is more element to iterate.
 *
 * @param iter iterator
 * @return true if there is more element to iterate
 */
bool shmmap_iterator_has_next(ShmmapIterator* iter);

/**
 * Get next element from iterator.
 *
 * @param iter iterator
 * @return next element (ShmmapEntry)
 */
ShmmapEntry* shmmap_iterator_next(ShmmapIterator* iter);

/**
 * Remove the element from the HashShmmap which is recetly iterated using shmmap_iterator_next function.
 *
 * @param iter iterator
 * @return removed element (ShmmapEntry)
 */
ShmmapEntry* shmmap_iterator_remove(ShmmapIterator* iter);

#define SHMMAP_HASH_TYPE_UINT64		1
#define SHMMAP_HASH_TYPE_STRING		2

#define SHMMAP_EQUALS_TYPE_UINT64	1
#define SHMMAP_EQUALS_TYPE_STRING	2

#endif /* __UTIL_MAP_H__ */
