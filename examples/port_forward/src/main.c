#include <stdio.h>
#include <thread.h>
#include <malloc.h>
#include <string.h>
#include <util/cmd.h>
#include <util/types.h>
#include <readline.h>
#include <net/nic.h>
#include <net/packet.h>
#include <net/ether.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/icmp.h>
#include <net/checksum.h>
#include <net/udp.h>

void ginit(int argc, char** argv) {
}

NIC** ni;
uint32_t count;
bool is_continue;

int8_t * port_map;

int init(int argc, char** argv) {
	cmd_init();
	count = nic_count();
	if(count < 4)
		return -1;

	ni = (NIC**)malloc(sizeof(NIC*) * count);
	for(int i = 0; i < count; i++) {
		ni[i] = nic_get(i);
	}

	port_map = (int8_t*)malloc(sizeof(int8_t) * count);
	memset(port_map, -1, sizeof(int8_t) * count);

	port_map[0] = 1;
	port_map[1] = 0;
	port_map[2] = 3;
	port_map[3] = 2;

	is_continue = true;

	return 0;
}

void destroy() {
	free(ni);
	free(port_map);
}

void gdestroy() {
}

static int cmd_exit(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	is_continue = false;

	return 0;
}

static int cmd_forward(int argc, char** argv, void(*callback)(char* result, int exit_status)) {
	if(argc == 1) {
		printf("**Port Map**\n");
		printf("In\tOut\n");
		for(int i = 0; i < count; i++) {
			printf("%d\t%d\n", i, port_map[i]);
		}
		printf("************\n");

		return 0;
	}

	if(argc == 2) {
		if(!is_uint8(argv[1]))
			return -1;

		int in = parse_uint8(argv[1]);
		printf("**Port Map**\n");
		printf("In\tOut\n");
		printf("%d\t%d\n", in, port_map[in]);
		printf("************\n");

		return 0;
	}

	if(argc == 3) {
		if(!is_uint8(argv[1]))
			return -1;

		if(!is_uint8(argv[2]))
			return -2;

		uint8_t in = parse_uint8(argv[1]);
		uint8_t out = parse_uint8(argv[2]);
		printf("%d %d\n", in, out);

		port_map[in] = out;

		return 0;
	}

	return -1;
}

Command commands[] = {
	{
		.name = "help",
		.desc = "Show this message",
		.func = cmd_help
	},
	{
		.name = "exit",
		.desc = "exit application",
		.func = cmd_exit
	},
	{
		.name = "forward",
		.desc = "pirnt forward map, setting forward",
		.args = "[uint8_t source] [uint8_t dest]",
		.func = cmd_forward
	},
	{
		.name = NULL,
		.desc = NULL,
		.args = NULL,
		.func = NULL
	},
};

uint32_t configure(char* cmd_line) {
	if(cmd_line == NULL)
		return -1;

	cmd_exec(cmd_line, NULL);

	return 0;
}

int main(int argc, char** argv) {
	printf("Thread %d booting\n", thread_id());
	if(thread_id() == 0) {
		ginit(argc, argv);
	}
	
	thread_barrior();
	
	if(init(argc, argv) < 0)
		return 0;
	
	thread_barrior();
	
	uint32_t count = nic_count();
	printf("nic count : %d\n", count);
	while(is_continue) {
		for(int i = 0; i < count; i++) {
			if(nic_has_input(ni[i])) {
				printf("in first if\n");
				Packet* packet = nic_input(ni[i]);
				if(packet == NULL)
					continue;

				if(port_map[i] != -1) {
					printf("packet here!\n");
					nic_output(ni[port_map[i]], packet);
					packet = NULL;
				}

				if(packet)
					nic_free(packet);
			}
		}

		//TODO setting
		char* line = readline();
		configure(line);
	}
	
	thread_barrior();
	
	destroy();
	
	thread_barrior();
	
	if(thread_id() == 0) {
		gdestroy(argc, argv);
	}
	
	return 0;
}
