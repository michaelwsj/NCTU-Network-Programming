#include <stdio.h>	// perror()
#include <stdlib.h>	// exit()
#include <string.h>	// bzero()
#include <sys/types.h>	// size_t, ssize_t
#include <unistd.h>	// read(), write(), close()
#include <errno.h>	// errno
#include <sys/socket.h>	// socket(), bind(), listen(), accept()
#include <sys/select.h>	// select()
#include <sys/time.h>	// select()
#include <netinet/in.h>	// htonl(), htons()
#include <arpa/inet.h>	// inet_pton()
#define MAXLINE 100
#define SERV_PORT 7777
#define SA struct sockaddr
#define LISTENQ 1024

void err_sys(const char* x){	// Fatal error related to a system call
	perror(x);
	exit(1);
}

void err_quit(const char* x){	// Fatal error unrelated to a system call
	perror(x);
	exit(1);
}

/*****	Error Handling Wrappers *****/
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
//	else
//		printf("Bind Success!\n");

	return n;
}

int Listen(int sockfd, int backlog){
	int n;

	if((n = listen(sockfd, backlog)) < 0)
		err_sys("Listen Error");
//	else
//		printf("Listen Success!\nbacklog: %d\n", backlog);

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
//	else
//		printf("Accept Success!\n");

	return connfd;
}

ssize_t writen(int fd, const void *vptr, size_t n){     // write n bytes to a descriptor
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

ssize_t readline(int fd, void *vptr, size_t maxlen){    // read a text line from a descriptor
        ssize_t n, rc;
        char c, *ptr;

        ptr = vptr;
        for(n = 1; n < maxlen; n++){
                if((rc = read(fd, &c, 1)) == 1){
                        *ptr++ = c;
                        if(c == '\n')
                                break;
                }
                else if(rc == 0){       // EOF
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

int main(int argc, char **argv){
	int i, j, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE], cli_port[FD_SETSIZE];
	char *cli_ip[FD_SETSIZE], *username[FD_SETSIZE];
	char prename[MAXLINE];
	ssize_t n;
	fd_set rset, allset;
	char line[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	char cmd[MAXLINE], arg[MAXLINE];
	char tellrcvr[MAXLINE], msg[MAXLINE];

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	// 0.0.0.0
	servaddr.sin_port = htons(SERV_PORT);

	Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	maxfd = listenfd;
	maxi = -1;
	for(i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for(;;){
		rset = allset;
		nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);

			for(i = 0; i < FD_SETSIZE; i++)
				if(client[i] < 0){
					client[i] = connfd;
					break;
				}

			if(i == FD_SETSIZE)
				err_quit("Too many clients");

			cli_ip[i] = (char *)malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip[i], INET_ADDRSTRLEN);
			cli_port[i] = ntohs(cliaddr.sin_port);
			username[i] = malloc(MAXLINE * sizeof(char));
			strcpy(username[i], "anonymous");

//			#ifdef NOTDEF
				printf("new client: %s, port: %d\n", cli_ip[i], cli_port[i]);
//			#endif

			FD_SET(connfd, &allset);

			if(connfd > maxfd)
				maxfd = connfd;

			if(i > maxi)
				maxi = i;

			for(j = 0; j <= maxi; j++){
				if(client[j] == connfd){
					sprintf(line, "[Server] Hello, %s! From %s/%d\n", username[j], cli_ip[j], cli_port[j]);
					write(client[j], line, sizeof(line));
				}
				else if (client[j] >= 0){
					strcpy(line, "[Server] Someone is coming!\n");
					write(client[j], line, sizeof(line));
				}
			}

			if(--nready <= 0)
				continue;
		}

		for(i = 0; i <= maxi; i++){
			if((sockfd = client[i]) < 0)
				continue;

			if(FD_ISSET(sockfd, &rset)){
				if((n = readline(sockfd, line, MAXLINE)) == 0){
					printf("Connection closed by client %s.\n", username[i]);

					for(j = 0; j <= maxi; j++){
						if(client[j] == sockfd){
							strcpy(line, "[Server] You are offline.\n");
							write(client[j], line, sizeof(line));
						}
						else if(client[j] >= 0){
							sprintf(line, "[Server] %s is offline.\n", username[i]);
							write(client[j], line, sizeof(line));
						}
					}

					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					free(cli_ip[i]);
					free(username[i]);
				}
				else{
					if(strcmp(line, "\n") == 0){
						write(sockfd, "\n", sizeof("\n"));
						continue;
					}

					strcpy(cmd, strtok(line, " "));
					if(cmd[strlen(cmd) - 1] == '\n')
						cmd[strlen(cmd) - 1] = '\0';

					strcpy(arg, line + strlen(cmd) + 1);
					if(arg[strlen(arg) - 1] == '\n')
						arg[strlen(arg) - 1] = '\0';

//					printf("cmd: %s, arg: %s\n", cmd, arg);

					if(strcmp(cmd, "who") == 0){
						for(j = 0; j <= maxi; j++){
							if(client[j] == sockfd){
								sprintf(line, "[Server] %s %s/%d -> me\n", username[j], cli_ip[j], cli_port[j]);
								write(client[i], line, sizeof(line));
							}
							else if(client[j] >= 0){
								sprintf(line, "[Server] %s %s/%d\n", username[j], cli_ip[j], cli_port[j]);
								write(client[i], line, sizeof(line));
							}
						}
					}
					else if(strcmp(cmd, "name") == 0){
						if(strcmp(arg, "anonymous") == 0){
							strcpy(line, "[Server] ERROR: Username cannot be anonymous.\n");
							write(sockfd, line, sizeof(line));
							continue;
						}

						for(j = 0; j <= maxi; j++){
							if(strcmp(arg, username[j]) == 0 && client[j] != sockfd){
								sprintf(line, "[Server] %s has been used by others.\n", arg);
								write(sockfd, line, sizeof(line));
								break;
							}
						}
						if(j <= maxi)
							continue;
						
						if(strlen(arg) < 2 || strlen(arg) > 12){
							strcpy(line, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
							write(sockfd, line, sizeof(line));
							continue;
						}

						strcpy(prename, username[i]);
						strcpy(username[i], arg);

						for(j = 0; j <= maxi; j++){
							if(client[j] == sockfd){
								sprintf(line, "[Server] You are now known as %s.\n", username[i]);
								write(client[j], line, sizeof(line));
							}
							else if(client[j] >= 0){
								sprintf(line, "[Server] %s is now known as %s.\n", prename, username[i]);
								write(client[j], line, sizeof(line));
							}
						}
					}
					else if(strcmp(cmd, "tell") == 0){
						if(strcmp(username[i], "anonymous") == 0){
							strcpy(line, "[Server] ERROR: You are anonymous.\n");
							write(sockfd, line, sizeof(line));
							continue;
						}

						strcpy(tellrcvr, strtok(arg, " "));
						if(tellrcvr[strlen(tellrcvr) - 1] == '\n')
							tellrcvr[strlen(tellrcvr) - 1] = '\0';

						strcpy(msg, arg + strlen(tellrcvr) + 1);
						if(msg[strlen(msg) - 1] == '\n')
							msg[strlen(msg) - 1] = '\0';

//						printf("tellrcvr: %s, msg: %s\n",tellrcvr, msg);

						if(strcmp(tellrcvr, username[i]) == 0){
							strcpy(line, "[Server] ERROR: You cannot send messages to yourself.\n");
							write(sockfd, line, sizeof(line));
							continue;
						}

						if(strcmp(tellrcvr, "anonymous") == 0){
							strcpy(line, "[Server] ERROR: The client to which you sent is anonymous.\n");
							write(sockfd, line, sizeof(line));
							continue;
						}

						for(j = 0; j <= maxi; j++){
							if(strcmp(tellrcvr, username[j]) == 0){
								sprintf(line, "[Server] %s tell you %s.\n", username[i], msg);
								write(client[j], line, sizeof(line));

								strcpy(line, "[Server] SUCCESS: Your message has been sent.\n");
								write(sockfd, line, sizeof(line));
								break;
							}
						}
						if(j > maxi){
							strcpy(line, "[Server] ERROR: The receiver doesn't exist.\n");
							write(sockfd, line, sizeof(line));
						}
					}
					else if(strcmp(cmd, "yell") == 0){
						for(j = 0; j <= maxi; j++){
							if(client[j] >= 0){
								sprintf(line, "[Server] %s yell %s.\n", username[i], arg);
								write(client[j], line, sizeof(line));
							}
						}
					}
					else if(strcmp(cmd, "check") == 0){
						for(j = 0; j <= maxi; j++){
							if(client[j] >= 0 && strcmp(username[j], arg) == 0){
								sprintf(line, "[Server] %s is online.\n", arg);
								write(sockfd, line, sizeof(line));
								break;
							}
						}
						if(j > maxi){
							sprintf(line, "[Server] %s is offline.\n", arg);
							write(sockfd, line, sizeof(line));
						}
					}
//					else if(strcmp(cmd, "exit") == 0){
//						close(sockfd);
//						FD_CLR(sockfd, &allset);
//						client[i] = -1;
//						free(cli_ip[i]);
//						free(username[i]);
//					}
					else{
						strcpy(line, "[Server] ERROR: Error command.\n");
						write(sockfd, line, sizeof(line));
					}
				}

				if(--nready <= 0)
					break;
			}
		}
	}

	return 0;
}
