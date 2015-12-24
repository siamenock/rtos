#include <stdio.h>
#include <string.h>
#include <thread.h>
#include <net/ni.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/tcp.h>
#include <util/list.h>
#include <util/event.h>
#include <gmalloc.h>
#include <readline.h>

//static uint32_t address = 0xc0a8640a;		// 192.168.100.10
//static uint64_t nat_mac = 0x909f33354bb0;  

typedef struct {
	char keyword[30];
	char alert_level[10];
} Keyword;

List* list;
void ginit(int argc, char** argv) {
}

void init(int argc, char** argv) {
}

char *strstr_t(const char *str1, const char *str2, int len) {
	char *cp = (char *) str1;
	char *s1, *s2;

	if (!*str2) return (char *) str1;

	for(int i = 0; i < len; i++) {
		s1 = cp;
		s2 = (char *) str2;

		while (*s1 && *s2 && !(*s1 - *s2)) s1++, s2++;
		if (!*s2) return cp;
		cp++;
	}

	return NULL;
}

void process(NetworkInterface* ni) {
	Packet* packet = ni_input(ni);
	static uint32_t source, destination;

	if(!packet)
		return;
	Ether* ether = (Ether*)(packet->buffer + packet->start);
	
	if(endian16(ether->type) == ETHER_TYPE_IPv4) {
		IP* ip = (IP*)ether->payload;

		if(ip->protocol == IP_PROTOCOL_TCP) {

			ListIterator iter;
			list_iterator_init(&iter, list);	
			while(list_iterator_has_next(&iter)) {
				Keyword* keyword = list_iterator_next(&iter);

				if(source != ip->source || destination != ip->destination) {
					if(strstr_t((const char*)ip->body, keyword->keyword, endian16(ip->length) - IP_LEN) != NULL) {
						source = ip->source;
						destination = ip->destination;
						printf("alert %s %s ", keyword->keyword, keyword->alert_level);
						printf("%d.%d.%d.%d ", (ip->source >> 24) & 0xff, (ip->source >> 16) & 0xff, (ip->source >> 8) & 0xff, (ip->source >> 0) & 0xff);
						printf("%d.%d.%d.%d\n", (ip->destination >> 24) & 0xff, (ip->destination >> 16) & 0xff, (ip->destination >> 8) & 0xff, (ip->destination >> 0) & 0xff);
					}
				}
			}
		}
	}

	

	ni_output(ni, packet);

	packet = NULL;
}


void destroy() {
}

void gdestroy() {
}

char* parsing(char* line) {

	char* token = NULL;
	char parsing[] = " ";

	token = strtok(line, parsing);

	return token;
}

static bool iterator(int context, char* word) {
	ListIterator iter;
	list_iterator_init(&iter, list);
	while(list_iterator_has_next(&iter)) {
		Keyword* keyword = list_iterator_next(&iter);

		// 1 : list, 2: add, 3 : remove
		if(context == 1) {
			printf("%s ", keyword->keyword);
			printf("%s\n", keyword->alert_level);
		}

		else {
			if(!strcmp(keyword->keyword, word)) {
				if(context == 2)
					return false;

				list_remove_data(list, keyword);
				gfree(keyword);
			}
		}
	}

	return true;
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	init(argc, argv);
	
	thread_barrior();
	
	uint32_t i = 0;

	list = list_create(NULL);
	
	while(1) {

		uint32_t count = ni_count();

		if(count > 0) {
			i = (i + 1) % count;

			NetworkInterface* ni = ni_get(i);

			if(ni_has_input(ni)) {
				process(ni);
			}
		}

		// Read stdin message and parse
		char* line = readline();
		if(line) {

			char* token = parsing(line);

			if(!strcmp(token, "list")) {
				// Print all keywords in the list
				iterator(1, NULL);

			} else if(!strcmp(token, "size")) {
				printf("size %d\n", list_size(list));

			} else if(!strcmp(token, "add")) {
				Keyword* keyword = gmalloc(sizeof(Keyword));

				strcpy(keyword->keyword, strtok(NULL, " "));
				strcpy(keyword->alert_level, strtok(NULL, " "));

				int err = iterator(2, keyword->keyword);

				// Add keyword into the list
				if(err)
					list_add(list, keyword);

			} else if(!strcmp(token, "remove")) {
				char* cmp = strtok(NULL, " ");

				// Find the keyword and remove
				iterator(3, cmp);
			}
		}
	}
	
	thread_barrior();
	
	destroy();
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
