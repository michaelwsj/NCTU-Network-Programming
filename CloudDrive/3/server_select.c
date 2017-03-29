#include "server.h"

int main(void){
	int listenfd, sockfd, connfd, datalistenfd, datafd, filefd;
	int maxi, maxfd, nready;
	struct sockaddr_in cliaddr, servaddr, dataaddr;
	socklen_t servlen, clilen;

	fd_set rset, allset;

	int client[FD_SETSIZE];
	int cli_port[FD_SETSIZE], data_port;
	char *cli_ip[FD_SETSIZE];
	
	char sendline[MAXLINE], recvline[MAXLINE], recvline_cp[MAXLINE];

	char *sourcefile, *targetfile;
	char *cmd;
	FILE *frptr, *fwptr;
	int n;

	int filenum[FD_SETSIZE];
	char *filelist[FD_SETSIZE][MAXFILE];
	
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	// 0.0.0.0
	servaddr.sin_port = htons((int)SERV_PORT);

	Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
	
	Listen(listenfd, LISTENQ);

	maxfd = listenfd;
	maxi = -1;
	for(int i = 0; i < FD_SETSIZE; i++){
		client[i] = -1;
		filenum[i] = 0;
	}
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	while(1){
		rset = allset;
		nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);

			for(n = 0; n < FD_SETSIZE; n++)
				if(client[n] < 0){
					client[n] = connfd;
					break;
				}

			if(n == FD_SETSIZE)
				err_quit("Too many clients");

			cli_ip[n] = (char *)malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip[n], INET_ADDRSTRLEN);
			cli_port[n] = ntohs(cliaddr.sin_port);

//			#ifdef NOTDEF
			printf("new client: %s, port: %d\n", cli_ip[n], cli_port[n]);
//			#endif

			FD_SET(connfd, &allset);

			if(connfd > maxfd)
				maxfd = connfd;

			if(n > maxi)
				maxi = n;

			if(--nready <= 0)
				continue;
		}

		for(int i = 0; i <= maxi; i++){
			if((sockfd = client[i]) < 0)
				continue;

			if(FD_ISSET(sockfd, &rset)){
				if((n = read(sockfd, recvline, MAXLINE)) == 0){
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;

					free(cli_ip[i]);
					cli_ip[i] = NULL;

					for(int j = 0; j < filenum[i]; j++){
						free(filelist[i][j]);
						filelist[i][j] = 0;
					}
					filenum[i] = 0;

					printf("Connection closed by client.\n");
					continue;
				}

				strcpy(recvline_cp, recvline);

				cmd = strtok(recvline_cp, " \n");
				sourcefile = strtok(NULL, " \n");
				targetfile = strtok(NULL, " \n");

				if(strcmp(cmd, "PUT") == 0){
					datalistenfd = Socket(AF_INET, SOCK_STREAM, 0);

					bzero(&dataaddr, sizeof(dataaddr));
					dataaddr.sin_family = AF_INET;
					dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
					servaddr.sin_port = 0;
//					servaddr.sin_port = htons((int)DATA_PORT);
	
					Bind(datalistenfd, (SA *)&servaddr, sizeof(servaddr));
					servlen = sizeof(servaddr);
					if(getsockname(datalistenfd, (SA *)&servaddr, &servlen) == -1)
						err_sys("getsockname Error");
	
					data_port = ntohs(servaddr.sin_port);
	
//					printf("data transferring port: %d.\n", data_port);
//					fflush(stdout);
	
					Listen(datalistenfd, LISTENQ);
	
					sprintf(sendline, "PUT %d", data_port);
					write(sockfd, sendline, sizeof(sendline));
	
					clilen = sizeof(dataaddr);
					datafd = Accept(datalistenfd, (SA *)&cliaddr, &clilen);
	
					if((fwptr = fopen(targetfile, "w")) == NULL){
						printf("Error: creating file failed.\n");
						continue;
					}
	
					filefd = fileno(fwptr);

					filelist[i][filenum[i]] = (char *)malloc(MAXLINE * sizeof(char));	
					strcpy(filelist[i][filenum[i]], targetfile);
					filenum[i]++;
	
					memset(recvline, '\0', MAXLINE);
					while((n = read(datafd, recvline, MAXLINE)) != 0){
//						fputs(recvline, fwptr);
						write(filefd, recvline, n);
						memset(recvline, '\0', MAXLINE);
					}
	
					close(filefd);
					fclose(fwptr);
	
					close(datafd);
	
					sprintf(sendline, "PUT %s %s succeeded.\n", sourcefile, targetfile);
					write(sockfd, sendline, sizeof(sendline));
				}
				else if(strcmp(cmd, "GET") == 0){
					datalistenfd = Socket(AF_INET, SOCK_STREAM, 0);
	
					bzero(&dataaddr, sizeof(dataaddr));
					dataaddr.sin_family = AF_INET;
					dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
					servaddr.sin_port = 0;
	
					Bind(datalistenfd, (SA *)&servaddr, sizeof(servaddr));

					servlen = sizeof(servaddr);	
					if(getsockname(datalistenfd, (SA *)&servaddr, &servlen) == -1)
						err_sys("getsockname Error");
	
					data_port = ntohs(servaddr.sin_port);
	
//					printf("data transferring port: %d.\n", data_port);
//					fflush(stdout);
	
					Listen(datalistenfd, LISTENQ);
	
					sprintf(sendline, "GET %d", data_port);
					write(sockfd, sendline, sizeof(sendline));
	
					clilen = sizeof(dataaddr);
					datafd = Accept(datalistenfd, (SA *)&cliaddr, &clilen);
	
					for(n = 0; n < filenum[i]; n++)
						if(strcmp(filelist[i][n], sourcefile) == 0)
							break;
					if(n == filenum[i]){
						printf("Error: file not found.\n");
						continue;
					}
	
					if((frptr = fopen(filelist[i][n], "r")) == NULL){
						printf("Error: creating file failed.\n");
						continue;
					}

					filefd = fileno(frptr);
	
//					while(fgets(sendline, MAXLINE, frptr) != NULL)
					while((n = read(filefd, sendline, MAXLINE)) != 0)
						write(datafd, sendline, n);
	
					close(filefd);
					fclose(frptr);
	
					shutdown(datafd, SHUT_WR);
	
					sprintf(sendline, "GET %s %s succeeded.\n", sourcefile, targetfile);
					write(sockfd, sendline, sizeof(sendline));
				}
				else if(strcmp(cmd, "LIST") == 0){
					strcpy(sendline, "LIST");
					write(sockfd, sendline, sizeof(sendline));
	
					for(int j = 0; j < filenum[i]; j++){
						sprintf(sendline, "%s\n", filelist[i][j]);
						write(sockfd, sendline, MAXLINE);
					}
	
					strcpy(sendline, "LIST succeeded.\n");
					write(sockfd, sendline, MAXLINE);
				}
				else{
					strcpy(sendline, "Error: invalid command.\n");
					write(sockfd, sendline, sizeof(sendline));
				}

				if(--nready <= 0)
					break;
			}	// end of if statement
		}	// end of for loop
	}	// end of while loop

	return 0;
}
