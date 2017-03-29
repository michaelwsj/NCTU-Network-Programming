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

int Connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen){
	int n;

	if((n = connect(sockfd, servaddr, addrlen)) < 0)
		err_sys("Connect Error");

	return n;
}

int Select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout){
	int fdcnt;

	if((fdcnt = select(maxfdp1, readset, writeset, exceptset, timeout)) < 0)
		err_sys("Select Error");

	return fdcnt;
}
