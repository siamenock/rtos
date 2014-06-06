#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <util/list.h>

static bool get_first_bigger(void* time, void* event) {
        return (uint64_t)time > (uint64_t)event;
}

void dump(List* list) {
	printf("\n");
	for(int i = 0; i < list_size(list); i++) {
		printf("[%d] %ld\n", i, (uint64_t)list_get(list, i));
	}
}

void main() {
	List* list = list_create(malloc, free);
	
	int idx = list_index_of(list, (void*)1, get_first_bigger);
	printf("idx = %d\n", idx);
	list_add(list, (void*)1);
	idx = list_index_of(list, (void*)2, get_first_bigger);
	printf("idx = %d\n", idx);
	list_add(list, (void*)2);
	idx = list_index_of(list, (void*)1, get_first_bigger);
	printf("idx = %d\n", idx);
	list_add_at(list, 0, (void*)10);
	dump(list);
	list_add_at(list, 0, (void*)20);
	dump(list);
	list_add_at(list, 1, (void*)30);
	dump(list);
	list_add_at(list, 100, (void*)50);
	dump(list);
	printf("%d\n", list_remove_data(list, (void*)30));
	dump(list);
}
