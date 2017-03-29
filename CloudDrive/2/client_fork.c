#include "client.h"

int main(int argc, char **argv){
	if(argc != 3){
		fprintf(stderr, "Error: incorrect number of arguments.\n");
		fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
		exit(1);
	}

	char *servip;
	int servport, dataport;
	int sockfd, datafd, filefd;
	struct sockaddr_in servaddr, dataaddr;

	char sendline[MAXLINE], sendline_cp[MAXLINE], recvline[MAXLINE], recvline_cp[MAXLINE];

	char *sourcefile, *targetfile;
	char *cmd, *opcode, *dataport_str;
	FILE *frptr, *fwptr;
	int n;

	servip = argv[1];
	servport = atoi(argv[2]);

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(servport);
	inet_pton(AF_INET, servip, &servaddr.sin_addr);

	Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	while(1){
		if(fgets(sendline, MAXLINE, stdin) == NULL){
			close(sockfd);
			exit(0);
		}

		if(strcmp(sendline, "\n") == 0){
			printf("\n");
			continue;
		}

		strcpy(sendline_cp, sendline);

		cmd = strtok(sendline_cp, " \n");
		sourcefile = strtok(NULL, " \n");
		targetfile = strtok(NULL, " \n");

		if(strcmp(cmd, "EXIT") == 0){
			close(sockfd);
			exit(0);
		}

		write(sockfd, sendline, sizeof(sendline));
		
		read(sockfd, recvline, MAXLINE);

		strcpy(recvline_cp, recvline);

		opcode = strtok(recvline_cp, " \n");

		if(strcmp(opcode, "PUT") == 0){
			dataport_str = strtok(NULL, " \n");
			dataport = atoi(dataport_str);

			datafd = Socket(AF_INET, SOCK_STREAM, 0);

			bzero(&dataaddr, sizeof(dataaddr));
			dataaddr.sin_family = AF_INET;
			dataaddr.sin_port = htons(dataport);
			inet_pton(AF_INET, servip, &servaddr.sin_addr);

			Connect(datafd, (SA *)&dataaddr, sizeof(dataaddr));

			if((frptr = fopen(sourcefile, "r")) == NULL){
				printf("Error: opening file failed.\n");
				close(datafd);
				continue;
			}

			filefd = fileno(frptr);

			memset(sendline, '\0', MAXLINE);
//			while(fgets(sendline, MAXLINE, frptr) != NULL){
			while((n = read(filefd, sendline, MAXLINE)) != 0){
				write(datafd, sendline, n);
				memset(sendline, '\0', MAXLINE);
			}

			close(filefd);
			fclose(frptr);

			shutdown(datafd, SHUT_WR);

			read(sockfd, recvline, MAXLINE);
			fputs(recvline, stdout);
		}
		else if(strcmp(opcode, "GET") == 0){
			strcpy(dataport_str, strtok(NULL, "\n"));
			dataport = atoi(dataport_str);

			datafd = Socket(AF_INET, SOCK_STREAM, 0);

			bzero(&dataaddr, sizeof(dataaddr));
			dataaddr.sin_family = AF_INET;
			dataaddr.sin_family = AF_INET;
			dataaddr.sin_port = htons(dataport);
			inet_pton(AF_INET, servip, &servaddr.sin_addr);

			Connect(datafd, (SA *)&dataaddr, sizeof(dataaddr));

			if((fwptr = fopen(targetfile, "w")) == NULL){
				printf("Error: creating file failed.\n");
				continue;
			}

			filefd = fileno(fwptr);

			while((n = read(datafd, recvline, MAXLINE)) != 0)
				write(filefd, recvline, n);

			close(filefd);
			fclose(fwptr);

			close(datafd);

			read(sockfd, recvline, MAXLINE);
			fputs(recvline, stdout);
		}
		else if(strcmp(opcode, "LIST") == 0){
			while(1){
				read(sockfd, recvline, MAXLINE);
				fputs(recvline, stdout);

				if(strcmp(recvline, "LIST succeeded.\n") == 0)
					break;
			}
		}
	}

	return 0;
}
