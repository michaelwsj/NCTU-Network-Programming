#include "server.h"

int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr, "Error: incorrect number of arguments.\n");
		fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
	}

	int servport;
	int listenfd;
	struct sockaddr_in servaddr;

	const int on = 1;

	servport = atoi(argv[1]);

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(servport);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	/* server function */
	serverfunc(listenfd);

	close(listenfd);

	return 0;
}
