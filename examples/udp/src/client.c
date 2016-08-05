#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>

int main(int argc, char** argv) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		return -1;
	
	struct sockaddr_in addr;
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(0xc0a8640a);	   // 192.168.100.10
	addr.sin_port = htons(7);
	
	struct sockaddr addr2;
	socklen_t len2;
	
	char buf[1472];
	while(1) {
		printf("Send: ");
		fflush(stdout);
		fgets(buf, 1472, stdin);
		int len = strlen(buf);
		sendto(fd, buf, len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));
		
		ssize_t len3 = recvfrom(fd, buf, 1472, 0, &addr2, &len2);
		if(len3 < 0)
			break;
		
		printf("Recv: %s", buf);
		fflush(stdout);
	}
}
