#include <stdio.h>	// fprintf()
#include <stdlib.h>	// exit()
#include <string.h>	// bzero()
#include <sys/types.h>
#include <unistd.h>	// access(), fork(), alarm(), sleep(), stat()
#include <errno.h>	// perror
#include <sys/socket.h>	// socket(), connect()
#include <sys/select.h>	// select()
#include <sys/time.h>	// struct timeval
#include <netinet/in.h>	// htons(), htonl()
#include <arpa/inet.h>	// inet_pton()
#include <signal.h>	// signal()
#include <sys/wait.h>	// wait()
#include <fcntl.h>	// open(), fcntl()
#include <sys/stat.h>	// open(), stat()
#include <time.h>	// ctime()

extern void err_sys(const char *x);
extern void err_quit(const char *x);
extern int Socket(int family, int type, int protocol);
extern int Bind(int sockfd, const struct sockaddr *myaddr, socklen_t addrlen);
extern int Listen(int sockfd, int backlog);
extern int Select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset, struct timeval *timeout);
extern int Accept(int sockfd, struct sockaddr *cliaddr, socklen_t *addrlen);
extern int Connect(int sockfd, const struct sockaddr *servaddr, socklen_t addrlen);
