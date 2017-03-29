#include "header.h"

#define LISTENQ 1024
#define MAXLINE 1024
#define MAXUSER 15	// maximum users able to register
#define MAXSERV 15	// maximum users able to be served
			/*
			 * Default FD_SETSIZE is 64.
			 * Each user needs four file descriptors at most, frfd, fwfd, dtrfd, and dtwfd.
			 * So maximam number of users able to be served is (64 - 3) / 4, about 15.
			 */
#define MAXFILE 5	// maximum files for each user
#define DATALSTNPORT 2222

#define SA struct sockaddr
#define max(a, b) ({__typeof__ (a) _a; __typeof__ (b) _b; _a > _b ? _a : _b;})

extern void serverfunc(int sockfd);
