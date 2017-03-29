#include "receiver.h"

int main(int argc, char **argv){
	if(argc != 3){
		fprintf(stderr, "Error: incorrect number of arguments.\n");
		fprintf(stderr, "Usage: %s <port_number> <file_name>\n", argv[0]);
	}

	int sndport;
	char *filename;
	int sockfd;
	struct sockaddr_in sndaddr;

	const int on = 1;

	sndport = atoi(argv[1]);
	filename = argv[2];

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&sndaddr, sizeof(sndaddr));
	sndaddr.sin_family = AF_INET;
	sndaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	sndaddr.sin_port = htons(sndport);

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	Bind(sockfd, (SA *)&sndaddr, sizeof(sndaddr));

	/* reliable transfer */
	rdt_receive(sockfd, filename);

	close(sockfd);

	return 0;
}
