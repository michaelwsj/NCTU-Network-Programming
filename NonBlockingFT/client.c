#include "client.h"

int main(int argc, char **argv){
	if(argc != 4){
		fprintf(stderr, "Error: incorrect number of arguments.\n");
		fprintf(stderr, "Usage: %s <server_ip> <port_number> <username>\n", argv[0]);
	}

	char *servip, *username;
	int servport;
	int sockfd;
	struct sockaddr_in servaddr;
	char writeline[MAXLINE];

	servip = argv[1];
	servport = atoi(argv[2]);
	username = argv[3];

	printf("Welcome to the dropbox-like server! : %s\n", username);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);
	inet_pton(AF_INET, servip, &servaddr.sin_addr);

	Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	sprintf(writeline, "USER %s", username);
	write(sockfd, writeline, strlen(writeline));

	/* client function */
	clientfunc(sockfd, servip);

	close(sockfd);

	return 0;
}
