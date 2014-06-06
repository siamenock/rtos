#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define ADDRESS		"192.168.100.254"
#define PORT		80
#define BUFFER_SIZE	4086

static int exchange_data();
static int connect_server();
static void set_addr();
static int set_socket();
static void print_word(int thread_id, int fd, char* text_data);
static void parse_data(char* buffer);
static char* create_message();

static int sock;
static struct sockaddr_in addr;

int main(int argc, char** argv) {
	if(set_socket() < 0) {
		fprintf(stderr, "socket setting error\n");
		exit(-1);
	}
	set_addr();
	
	if(connect_server() < 0) {
		exit(-1);
	}
	
	exchange_data();

	return 0;
}

int exchange_data() {
	int fd_num;
	char buffer[BUFFER_SIZE] = { 0, };
	fd_set in_put, out_put;
	fd_set temp_in, temp_out;
	struct timeval nulltime;

	FD_ZERO(&in_put);
	FD_ZERO(&out_put);
	FD_SET(sock, &in_put);
	FD_SET(sock, &out_put);
	FD_SET(STDIN_FILENO, &in_put);

	while(1) {
		temp_in = in_put;
		temp_out = out_put;
		nulltime.tv_sec = 3;
		nulltime.tv_usec = 0;
		fd_num = select(sock + 1, &temp_in, &temp_out, 0, &nulltime);

		if(fd_num == -1) {
			printf("selector error \n");
			return -1;
		}
		if(fd_num == 0) {
			continue;
		} else{
			if(FD_ISSET(sock, &temp_in) != 0) {
				recv(sock, buffer, BUFFER_SIZE - 1, 0);
				parse_data(buffer);
				memset(buffer, 0, BUFFER_SIZE);
			} else if(FD_ISSET(STDIN_FILENO, &temp_in) != 0) {
				char* message = create_message();
				send(sock, message, strlen(message), 0);
				free(message);
			}
		}
	}
}

int connect_server() {
	char* header = "POST /vm/1/stdio HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n";
	char buffer[BUFFER_SIZE] = { 0, };

	if(connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "connect error %d\n", errno);
		return -1;
	}

	send(sock, header, strlen(header), 0);
	recv(sock, buffer, BUFFER_SIZE - 1, 0);
	printf("%s", buffer);

	return 0;
}

void set_addr() {
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ADDRESS);
	addr.sin_port = htons(PORT);
}

int set_socket() {
	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sock < 0) {
		fprintf(stderr, "socket error\n");
		return -1;
	}
}

char* create_message() {
	int total_size;
	int thread_id;
	int text_size;
	
	char t_text_size[4] = { 0, };
	char  t_thread_id[13] = { 0, };
	char t_total_size[5] = { 0, };

	char buffer[BUFFER_SIZE] = { 0, };
	
	char* chunked_data;
	
	scanf("%d", &thread_id);
	getc(stdin);
	gets(buffer);
	
	text_size = strlen(buffer) + 2;
	sprintf(t_text_size, "%0x", text_size);
	sprintf(t_thread_id, "%0x", thread_id);
	total_size = text_size + strlen(t_text_size) + 2 + 3 + strlen(t_thread_id) + 2;
	sprintf(t_total_size, "%0x", total_size);

	chunked_data = (char*)malloc(sizeof(char) * (total_size + strlen(t_total_size) + 5));
	memset(chunked_data, 0, sizeof(char) * (total_size + strlen(t_total_size) + 5));	
	sprintf(chunked_data, "%s\r\n%s\r\n0\r\n%s\r\n%s\r\n\r\n", t_total_size, t_thread_id, t_text_size, buffer);

	return chunked_data;
}

void parse_data(char* buffer) {
	int total_size;
	int thread_id;
	int fd;
	int text_size;
	
	char* text_data;
	char t_head[40] = { 0, };
	int head_size;

	while(1) {
		sscanf(buffer, "%0x %0x %0x %0x", &total_size, &thread_id, &fd, &text_size);
		sprintf(t_head, "%0x\r\n%0x\r\n%0x\r\n%0x\r\n", total_size, thread_id, fd, text_size);

		text_data = (char*)malloc(sizeof(char) * text_size + 1);
		memset(text_data, 0, sizeof(char) * text_size + 1);
		head_size = strlen(t_head);
		strncpy(text_data, buffer + head_size, text_size);
	
		print_word(thread_id, fd, text_data);
		free(text_data);

		strncpy(buffer, buffer + head_size + text_size + 2, strlen(buffer) - head_size - text_size);
		if(strlen(buffer) == 0)
			break;
		if(strncmp(buffer, "\r\n", 2) == 0)
			break;
	}
}

void print_word(int thread_id, int fd, char* text_data) {
	switch(fd) {
		case 0:
			printf("%d sin> %s", thread_id, text_data);
			break;

		case 1:
			printf("%d err> %s", thread_id, text_data);
			break;

		case 2:
			printf("%d out> %s", thread_id, text_data);
			break;
	}
}
