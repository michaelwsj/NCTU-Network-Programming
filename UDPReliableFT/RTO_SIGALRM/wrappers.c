#include "header.h"

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

	/* handle EINTR error */
/*
	do{
		fdcnt = select(maxfdp1, readset, writeset, exceptset, timeout);
	}while(fdcnt < 0 && errno == EINTR);

	if(fdcnt < 0)
		err_sys("Select Error");
*/

// Note: select() is never restart after being interrupted by a system call

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

int Connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen){
	int n;

	if((n = connect(sockfd, servaddr, addrlen)) < 0)
		err_sys("Connect Error");

	return n;
}
