#include "header.h"

#define SERV_PORT 7777
#define DATA_PORT 7778	// for fixed data transferring port
#define LISTENQ 1024

#define MAXFILE 20

void err_sys(const char* x){
	perror(x);
	exit(1);
}

void err_quit(const char* x){
	perror(x);
	exit(1);
}

int Socket(int family, int type, int protocol){
	int sockfd;

	if((sockfd = socket(family, type, protocol)) < 0)
		err_sys("Socket Error");

	return sockfd;
}

int Bind(int sockfd, const struct sockaddr *myaddr, socklen_t addrlen){
	int n;

	if((n = bind(sockfd, myaddr, addrlen)) < 0)
		err_sys("Bind Error");

	return n;
}

int Listen(int sockfd, int backlog){
	int n;

	if((n = listen(sockfd, backlog)) < 0)
		err_sys("Listen Error");

	return n;
}

int Select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout){
	int fdcnt;

	if((fdcnt = select(maxfdp1, readset, writeset, exceptset, timeout)) < 0)
		err_sys("Select Error");

	return fdcnt;
}

int Accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen){
	int connfd;

	if((connfd = accept(sockfd, cliaddr, addrlen)) < 0)
		err_sys("Accept Error");

	return connfd;
}
