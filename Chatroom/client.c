#include <stdio.h>	// perror()
#include <stdlib.h>	// exit()
#include <string.h>	// bzero()
#include <sys/types.h>	// size_t, ssize_t
#include <unistd.h>	// read(), write()
#include <errno.h>	// errno
#include <sys/socket.h>	// socket(), connect(), shutdown()
#include <sys/select.h>	// select()
#include <sys/time.h>	// select()
#include <netinet/in.h>	// htons()
#include <arpa/inet.h>	// inet_pton()
#define MAXLINE 100
#define SA	struct sockaddr
#define max(a, b) ({ __typeof__ (a) _a; __typeof__ (b) _b = (b); _a > _b ? _a : _b;})

void err_sys(const char* x){	// Fatal error related to a system call
	perror(x);
	exit(1);
}

void err_quit(const char* x){	// Fatal error unrelated to a system call
	perror(x);
	exit(1);
}

/*****	Error Handling Wrappers	*****/
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

ssize_t writen(int fd, const void *vptr, size_t n){	// write n bytes to a descriptor
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft)) <= 0){
			if(nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}
		
		nleft -= nwritten;
		ptr += nwritten;
	}

	return n;
}

ssize_t readline(int fd, void *vptr, size_t maxlen){	// read a text line from a descriptor
	ssize_t n, rc;
	char c, *ptr;

	ptr = vptr;
	for(n = 1; n < maxlen; n++){
		if((rc = read(fd, &c, 1)) == 1){
			*ptr++ = c;
			if(c == '\n')
				break;
		}
		else if(rc == 0){	// EOF
			*ptr = 0;
			return (n - 1);
		}
		else{
			if(errno == EINTR){
				n--;
				continue;
			}
			else
				return -1;
		}
	}

	*ptr = 0;

	return n;
}

void chat_cli(FILE *fp, int sockfd){
	int maxfdp1, stdineof;
	fd_set rset;
	char sendline[MAXLINE], recvline[MAXLINE];

	stdineof = 0;
	FD_ZERO(&rset);

	for(;;){
		if(stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;

		Select(maxfdp1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &rset)){	// socket is readable
			if(read(sockfd, recvline, MAXLINE) == 0){
				if(stdineof == 1)
					return;
				else
					err_quit("Server Terminated");
			}
			fputs(recvline, stdout);
		}

		if(FD_ISSET(fileno(fp), &rset)){	// input is readable
			if(fgets(sendline, MAXLINE, fp) == NULL || strcmp(sendline, "exit\n") == 0){
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			write(sockfd, sendline, strlen(sendline));
		}
	}
}

int main(int argc, char **argv){
	char *servip;
	int servport;
	int sockfd;
	struct sockaddr_in servaddr;

	if(argc != 3){
		printf("Incorrect inputs. Usage: ./client <Server IP> <Port Number>.\n");
		exit(1);
	}

	servip = argv[1];
	servport = atoi(argv[2]);

//	printf("Server ip: %s\nServer port: %d\n", servip, servport);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));	// set bytes of servaddr to zero
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);
	inet_pton(AF_INET, servip, &servaddr.sin_addr);

	Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	chat_cli(stdin, sockfd);

	return 0;
}
