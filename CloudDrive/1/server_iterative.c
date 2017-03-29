#include "server.h"

int main(void){
	int listenfd, sockfd, datalistenfd, datafd, filefd;
	struct sockaddr_in cliaddr, servaddr, dataaddr;
	socklen_t servlen, clilen;

	int cli_port, data_port;
	char cli_ip[INET_ADDRSTRLEN];

	char sendline[MAXLINE], recvline[MAXLINE], recvline_cp[MAXLINE];

	char *sourcefile, *targetfile;
	char *cmd;
	FILE *frptr, *fwptr;
	int n;

	int filenum = 0;
	char filelist[MAXFILE][MAXLINE];
	
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);	// 0.0.0.0
	servaddr.sin_port = htons((int)SERV_PORT);

	Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
	
	Listen(listenfd, LISTENQ);

	while(1){
		clilen = sizeof(cliaddr);
		sockfd = Accept(listenfd, (SA *)&cliaddr, &clilen);

		inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip, INET_ADDRSTRLEN);
		cli_port = ntohs(cliaddr.sin_port);

//		#ifdef NOTDEF
		printf("new client: %s, port: %d\n", cli_ip, cli_port);
//		#endif

		while(1){
			if((n = read(sockfd, recvline, MAXLINE)) == 0){
				printf("Connection closed by client.\n");
				break;
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
//				servaddr.sin_port = htons((int)DATA_PORT);

				Bind(datalistenfd, (SA *)&servaddr, sizeof(servaddr));
				servlen = sizeof(servaddr);
				if(getsockname(datalistenfd, (SA *)&servaddr, &servlen) == -1)
					err_sys("getsockname Error");

				data_port = ntohs(servaddr.sin_port);

//				printf("data transferring port: %d.\n", data_port);
//				fflush(stdout);

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

				strcpy(filelist[filenum], targetfile);
				filenum++;

				memset(recvline, '\0', MAXLINE);
				while((n = read(datafd, recvline, MAXLINE)) != 0){
//					fputs(recvline, fwptr);
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

				if(getsockname(datalistenfd, (SA *)&servaddr, &servlen) == -1)
					err_sys("getsockname Error");

				data_port = ntohs(servaddr.sin_port);

//				printf("data transferring port: %d.\n", data_port);
//				fflush(stdout);

				Listen(datalistenfd, LISTENQ);

				sprintf(sendline, "GET %d", data_port);
				write(sockfd, sendline, sizeof(sendline));

				clilen = sizeof(dataaddr);
				datafd = Accept(datalistenfd, (SA *)&cliaddr, &clilen);

				for(n = 0; n < filenum; n++)
					if(strcmp(filelist[n], sourcefile) == 0)
						break;
				if(n == filenum){
					printf("Error: file not found.\n");
					continue;
				}

				if((frptr = fopen(filelist[n], "r")) == NULL){
					printf("Error: creating file failed.\n");
					continue;
				}

				filefd = fileno(frptr);

//				while(fgets(sendline, MAXLINE, frptr) != NULL)
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

				for(int i = 0; i < filenum; i++){
					sprintf(sendline, "%s\n", filelist[i]);
					write(sockfd, sendline, MAXLINE);
				}

				strcpy(sendline, "LIST succeeded.\n");
				write(sockfd, sendline, MAXLINE);
			}
			else{
				strcpy(sendline, "Error: invalid command.\n");
				write(sockfd, sendline, sizeof(sendline));
			}
		}

		filenum = 0;
	}

	return 0;
}
