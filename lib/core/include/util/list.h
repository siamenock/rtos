#ifndef __UTIL_LIST_H__
#define __UTIL_LIST_H__

#include <stddef.h>
#include <stdbool.h>

typedef struct _ListNode {
	struct _ListNode*	prev;
	struct _ListNode*	next;
	void*			data;
} ListNode;

typedef struct _List {
	ListNode*	head;
	ListNode*	tail;
	size_t		size;
	void*		pool;
} List;

List* list_create(void* pool);
void list_destroy(List* list);
bool list_is_empty(List* list);
bool list_add(List* list, void* data);
bool list_add_at(List* list, int index, void* data);
void* list_get(List* list, int index);
void* list_get_first(List* list);
void* list_get_last(List* list);
int list_index_of(List* list, void* data, bool(*comp_fn)(void*,void*));
void* list_remove(List* list, int index);
bool list_remove_data(List* list, void* data);
void* list_remove_first(List* list);
void* list_remove_last(List* list);
size_t list_size(List* list);
void list_rotate(List* list);

typedef struct _ListIterator {
	List* list;
	ListNode* node;
} ListIterator;

void list_iterator_init(ListIterator* iter, List* list);
bool list_iterator_has_next(ListIterator* iter);
void* list_iterator_next(ListIterator* iter);
void* list_iterator_remove(ListIterator* iter);

#endif /* __UTIL_LIST_H__ */
