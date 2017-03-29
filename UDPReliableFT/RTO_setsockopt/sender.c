#include "sender.h"

int main(int argc, char **argv){
	if(argc != 4){
		fprintf(stderr, "Error: incorrect number of arguments.\n");
		fprintf(stderr, "Usage: %s <receiver_ip> <port_number> <file_name>\n", argv[0]);
	}

	char *rcvip, *filename;
	int rcvport;
	int sockfd;
	struct sockaddr_in rcvaddr, dataaddr;

	char sndline[MAXLINE], rcvline[MAXLINE];

	rcvip = argv[1];
	rcvport = atoi(argv[2]);
	filename = argv[3];

	bzero(&rcvaddr, sizeof(rcvaddr));
	rcvaddr.sin_family = AF_INET;
	rcvaddr.sin_port = htons(rcvport);
	inet_pton(AF_INET, rcvip, &rcvaddr.sin_addr);

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	/* reliable transfer */
	rdt_send(sockfd, (SA *)&rcvaddr, sizeof(rcvaddr), filename);

	close(sockfd);

	return 0;
}
