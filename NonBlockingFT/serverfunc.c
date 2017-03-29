/* server function */
#include "server.h"

int usercnt, maxi, client[MAXSERV], filecnt[MAXSERV];
char clientlist[MAXSERV][MAXLINE]; 
char userlist[MAXUSER][MAXLINE];
char *filelist[MAXUSER][MAXFILE];

/* blocking write on nonblocking socket */
int blockingwrite(int fd, char *str, int len){
	int n;
	char *ptr = str;

	do{
		n = write(fd, str, &str[len] - ptr);	// nonblocking write
		ptr += n;
	}while((n < 0 && errno == EWOULDBLOCK) || ptr != &str[len]);

	if(n < 0)
		return -1;
	return len;
}

int blockingread(int fd, char *str, int len){
	int n;

	do{
		n = read(fd, str, MAXLINE);	// nonblocking read
	}while(n < 0 && errno == EWOULDBLOCK);

	if(n < 0)
		return -1;

	return n;
}

char* gf_time(void){
	struct timeval tv;
	static char str[30];
	char *ptr;

	if(gettimeofday(&tv, NULL) < 0)
		err_sys("gettimeofday error");

	ptr = ctime(&tv.tv_sec);
	strcpy(str, &ptr[11]);
	snprintf(str + 8, sizeof(str) - 8, ".%06ld", tv.tv_sec);

	return str;
}

void printlist(void){	// debug code: print clientlist and corresponding files
	int i, j, k;

	for(i = 0; i <= maxi; i++){
		if(client[i] < 0)
			continue;

		printf("client %d %s\n", i, clientlist[i]);

		if(filecnt[i] == 0){
			printf("\tNo put file\n");
			continue;
		}

		for(j = 0; j < usercnt; j++)
			if(strcmp(clientlist[i], userlist[j]) == 0)
				break;

		for(k = 0; k < filecnt[i]; k++){
			printf("\t%s\n", filelist[j][k]);
		}
	}

	return;
}

void serverfunc(int listenfd){
	/*
	 * It seems more clean to create a struct for each connecting user.
	 * And a struct to record existing user.
	 * But it too late for me to realize it.
	 */
	int i, j, connfd;

	int frfd[MAXSERV], fwfd[MAXSERV], datafd[MAXSERV];
	int maxfdp1, val;
	int data_write[MAXSERV], data_read[MAXSERV], file_write[MAXSERV], file_read[MAXSERV];
	int n, nwritten;
	int exit[MAXSERV];
	fd_set rset, wset;

	char to[MAXSERV][MAXLINE], fr[MAXSERV][MAXLINE], readline[MAXLINE], writeline[MAXLINE];
	char *toiptr[MAXSERV], *tooptr[MAXSERV], *friptr[MAXSERV], *froptr[MAXSERV];

	int datalstnfd, dataport;
	struct sockaddr_in dataaddr, cliaddr;
	socklen_t datalen, clilen;

	struct stat filestat;
	int filesize, progresssize;

	char *cmd, *arg;

	const int on = 1;

	/* add O_NONBLOCK to file status */
	val = fcntl(listenfd, F_GETFL, 0);
	fcntl(listenfd, F_SETFL, val | O_NONBLOCK);

	/* initialize */
	for(i = 0; i < MAXSERV; i++){
		client[i] = -1;
		filecnt[i] = 0;
		exit[i] = 0;

		toiptr[i] = tooptr[i] = to[i];
		friptr[i] = froptr[i] = fr[i];

		data_write[i] = 0;
		data_read[i] = 0;
		file_write[i] = 0;
		file_read[i] = 0;
	}

	usercnt = 0;
	for(i = 0; i < MAXUSER; i++){
		for(j = 0; j < MAXFILE; j++)
			filelist[i][j] = NULL;
	}

	maxi = -1;

	// datalstn initialize
	datalstnfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&dataaddr, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	dataaddr.sin_port = htons(DATALSTNPORT);

	setsockopt(datalstnfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	Bind(datalstnfd, (SA *)&dataaddr, sizeof(dataaddr));

	Listen(datalstnfd, LISTENQ);

	for( ; ; ){
		for(i = 0; i <= maxi; i++){
			if(client[i] == -1)
				continue;

			if(exit[i] == 1 && data_write[i] == 0 && file_write[i] == 0){	// client exit
				fprintf(stderr, "%s: client %s exit\n", gf_time(), clientlist[i]);
				shutdown(client[i], SHUT_WR);
				client[i] = -1;
				filecnt[i] = 0;
				exit[i]	= 0;
			}
		}

		FD_ZERO(&rset);
		FD_ZERO(&wset);

		FD_SET(listenfd, &rset);
		maxfdp1 = listenfd + 1;

		for(i = 0; i <= maxi; i++){	// user-served index
			if(client[i] == -1)
				continue;

			FD_SET(client[i], &rset);
			maxfdp1 = (maxfdp1 < client[i] + 1) ? client[i] + 1 : maxfdp1;

			if(file_read[i] == 1 && toiptr[i] < &to[i][MAXLINE]){
				FD_SET(frfd[i], &rset);	// read from file
				maxfdp1 = (maxfdp1 < frfd[i] + 1) ? frfd[i] + 1 : maxfdp1;
			}
			if(data_write[i] == 1 && tooptr[i] != toiptr[i]){
				FD_SET(datafd[i], &wset);	// write to data socket
				maxfdp1 = (maxfdp1 < datafd[i] + 1) ? datafd[i] + 1 : maxfdp1;
			}

			if(data_read[i] == 1 && friptr[i] < &fr[i][MAXLINE]){
				FD_SET(datafd[i], &rset);	// read from data socket
				maxfdp1 = (maxfdp1 < datafd[i] + 1) ? datafd[i] + 1 : maxfdp1;
			}
			if(file_write[i] == 1 && froptr[i] != friptr[i]){
				FD_SET(fwfd[i], &wset);	// write to file
				maxfdp1 = (maxfdp1 < fwfd[i] + 1) ? fwfd[i] + 1 : maxfdp1;
			}
		}

		Select(maxfdp1, &rset, &wset, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
			clilen = sizeof(cliaddr);
			connfd = Accept(listenfd, (SA *)&cliaddr, &clilen);

			for(i = 0; i < MAXSERV; i++){
				if(client[i] < 0){
					client[i] = connfd;
					break;
				}
			}
			if(i == MAXSERV)
				err_quit("Too many clients");

			if(i > maxi)
				maxi = i;

			// exit[i] = 0;	// initialize exit flag

			n = read(client[i], readline, MAXLINE);
			readline[n] = '\0';

			cmd = strtok(readline, " ");
			arg = strtok(NULL, " ");

			if(strcmp(cmd, "USER") != 0)
				fprintf(stderr, "%s: error command\n", gf_time());

			fprintf(stderr, "%s: User %s connect\n", gf_time(), arg);

			// user identity certification
			strcpy(clientlist[i], arg);
			for(j = 0; j < usercnt; j++){	// user-registered index
				if(strcmp(arg, userlist[j]) == 0)
					break;
			}

			// data port initialize
			clilen = sizeof(cliaddr);
			datafd[i] = Accept(datalstnfd, (SA *)&cliaddr, &clilen);

			val = fcntl(datafd[i], F_GETFL, 0);
			fcntl(datafd[i], F_SETFL, val | O_NONBLOCK);

			if(j < usercnt){	// existing user
				if(filelist[j][0] != NULL){	// filelist transfer
					// file read set
					frfd[i] = open(filelist[j][0], O_RDONLY);

					val = fcntl(frfd[i], F_GETFL, 0);
					fcntl(frfd[i], F_SETFL, val | O_NONBLOCK);

					file_read[i] = 1;
					if(toiptr[i] < &to[i][MAXLINE])
						FD_SET(frfd[i], &rset);

					if(stat(filelist[j][0], &filestat) == -1)
							err_sys("stat error");
					filesize = filestat.st_size;

					printf("filesize: %d\n", filesize);

					sprintf(writeline, "SYN %s %d", filelist[j][0], filesize);
					write(client[i], writeline, strlen(writeline));

					printf("%send\n", writeline);
					fflush(stdout);

					// data socket write set
					data_write[i] = 1;
					if(toiptr[i] != tooptr[i])
						FD_SET(datafd[i], &wset);

					filecnt[i]++;
					progresssize = 0;
				}
			}
			else{	// new user
				if(usercnt == MAXUSER)
					err_quit("Too many users");

				strcpy(userlist[usercnt], arg);
				usercnt++;
			}

			// nonblocking setting
			val = fcntl(client[i], F_GETFL, 0);
			fcntl(client[i], F_SETFL, val | O_NONBLOCK);

			FD_SET(client[i], &rset);
		}

		for(i = 0; i <= maxi; i++){
			if(FD_ISSET(client[i], &rset)){
				if((n = read(client[i], readline, MAXLINE)) < 0){
					if(errno != EWOULDBLOCK)
						err_sys("read error on client socket");
				}
				else if(n == 0){	// client exit, shutdown write end of socket
					if(data_write[i] == 0 && file_write[i] == 0){
						fprintf(stderr, "%s: client %s exit\n", gf_time(), clientlist[i]);
						shutdown(client[i], SHUT_WR);	// shutdown write end too
						client[i] = -1;
						filecnt[i] = 0;
					}
					else
						exit[i] = 1;	// note as exit status
				}
				else{
					readline[n] = '\0';
					fprintf(stderr, "%s: read %d bytes from client socket\n", gf_time(), n);

					char *cmd, *arg;
					cmd = strtok(readline, " ");
					arg = strtok(NULL, " ");

					if(strcmp(cmd, "PUT") == 0){	// PUT <filename>
						char *arg2;
						arg2 = strtok(NULL, " ");
						filesize = atoi(arg2);
						// data socket read set
						data_read[i] = 1;
						if(friptr[i] != froptr[i])
							FD_SET(datafd[i], &rset);

						// file write set
						fwfd[i] = open(arg, O_WRONLY | O_CREAT | O_TRUNC, 0644);

						val = fcntl(fwfd[i], F_GETFL, 0);
						fcntl(fwfd[i], F_SETFL, val | O_NONBLOCK);

						file_write[i] = 1;
						if(friptr[i] < &fr[i][MAXLINE])
							FD_SET(fwfd[i], &wset);

						// filelist update
						for(j = 0; j < usercnt; j++){
							if(strcmp(userlist[j], clientlist[i]) == 0)
								break;
						}
						filelist[j][filecnt[i]] = malloc(MAXLINE * sizeof(char));
						strcpy(filelist[j][filecnt[i]], arg);

						filecnt[i]++;
						progresssize = 0;
					}
					else{
						fprintf(stderr, "%s: error command %s\n", gf_time(), cmd);
					}
				}
			}

			if(FD_ISSET(frfd[i], &rset)){
				if((n = read(frfd[i], toiptr[i], &to[i][MAXLINE] - toiptr[i])) < 0){
					if(errno != EWOULDBLOCK)
						err_sys("read error on file");
				}
				else if(n == 0){
					fprintf(stderr, "%s: EOF on reading file\n", gf_time());
					file_read[i] = 0;
					close(frfd[i]);
				}
				else{
					fprintf(stderr, "%s: read %d bytes from file\n", gf_time(), n);
					toiptr[i] += n;
					FD_SET(datafd[i], &wset);
				}
			}

			if(FD_ISSET(datafd[i], &wset) && ((n = toiptr[i] - tooptr[i]) > 0)){
				if((nwritten = write(datafd[i], tooptr[i], n)) < 0){
					if(errno != EWOULDBLOCK)
						err_sys("write error to data socket");
				}
				else{
					fprintf(stderr, "%s: wrote %d bytes to data socket\n", gf_time(), nwritten);
					tooptr[i] += nwritten;
					progresssize += nwritten;
					printf("tooptr: %d, toiptr: %d\n", tooptr[i], toiptr[i]);
					if(tooptr[i] == toiptr[i]){
						toiptr[i] = tooptr[i] = to[i];	// back to beginning of the buffer
						if(progresssize == filesize){
							fprintf(stderr, "%s: EOF on writing data socket\n", gf_time());
							data_write[i] = 0;

							// n = blockingread(client[i], readline, MAXLINE);
							// readline[n] = '\0';

							// cmd = strtok(readline, " ");
							// arg = strtok(NULL, " ");

							// if(strcmp(cmd, "SYN-DONE") == 0)
							// 	fprintf(stderr, "%s: SYN file %s done\n", gf_time(), arg);

							printlist();

							/*
							 * It is more reasonable to have server first send filelist when user connecting.
							 * And user request for file iteratively.
							 */
							for(j = 0; j < usercnt; j++){
								if(strcmp(userlist[j], clientlist[i]) == 0)
									break;
							}
							// if filelist is not empty
							if(filelist[j][filecnt[i]] != NULL){
								// file read set
								frfd[i] = open(filelist[j][filecnt[i]], O_RDONLY);

								val = fcntl(frfd[i], F_GETFL, 0);
								fcntl(frfd[i], F_SETFL, val | O_NONBLOCK);

								file_read[i] = 1;
								if(toiptr[i] < &to[i][MAXLINE])
									FD_SET(frfd[i], &rset);

								if(stat(filelist[j][filecnt[i]], &filestat) == -1)
										err_sys("stat error");
								filesize = filestat.st_size;

								sprintf(writeline, "SYN %s %d", filelist[j][filecnt[i]], filesize);
								blockingwrite(client[i], writeline, strlen(writeline));

								data_write[i] = 1;
								if(toiptr[i] != tooptr[i])
									FD_SET(datafd[i], &wset);

								filecnt[i]++;
								progresssize = 0;
							}
						}
					}
				}
			}

			if(FD_ISSET(datafd[i], &rset)){
				if((n = read(datafd[i], friptr[i], &fr[i][MAXLINE] - friptr[i])) < 0){
					if(errno != EWOULDBLOCK)
						err_sys("read error on data socket");
				}
				else if(n == 0){
					fprintf(stderr, "%s: EOF on reading data socket\n", gf_time());
				}
				else{
					fprintf(stderr, "%s: read %d bytes from data socket\n", gf_time(), n);
					friptr[i] += n;
					FD_SET(fwfd[i], &wset);
				}
			}

			if(FD_ISSET(fwfd[i], &wset) && ((n = friptr[i] - froptr[i]) > 0)){
				if((nwritten = write(fwfd[i], froptr[i], n)) < 0){
					if(errno != EWOULDBLOCK)
						err_sys("write error to file");
				}
				else{
					fprintf(stderr, "%s: wrote %d bytes to file\n", gf_time(), nwritten);
					froptr[i] += nwritten;
					progresssize += nwritten;
					if(froptr[i] == friptr[i]){
						friptr[i] = froptr[i] = fr[i];	// back to beginning of the buffer
						if(progresssize == filesize){
							fprintf(stderr, "%s: EOF on writing file\n", gf_time());
							data_read[i] = 0;
							file_write[i] = 0;
							close(fwfd[i]);	// end of writing file

							// file synchronization
							for(j = 0; j <= maxi; j++){
								if(j == i)
									continue;
								if(client[j] != -1 && strcmp(clientlist[i], clientlist[j]) == 0 && filecnt[j] < filecnt[i]){
									int k;
									for(k = 0; k < usercnt; k++){
										if(strcmp(userlist[k], clientlist[i]) == 0)
											break;
									}
									// file read set
									frfd[j] = open(filelist[k][filecnt[j]], O_RDONLY);

									val = fcntl(frfd[j], F_GETFL, 0);
									fcntl(frfd[j], F_SETFL, val | O_NONBLOCK);

									file_read[j] = 1;
									if(toiptr[j] < &to[j][MAXLINE])
										FD_SET(frfd[j], &rset);

									if(stat(filelist[k][filecnt[j]], &filestat) == -1)
											err_sys("stat error");
									filesize = filestat.st_size;

									sprintf(writeline, "SYN %s %d", filelist[k][filecnt[j]], filesize);
									blockingwrite(client[j], writeline, strlen(writeline));

									data_write[j] = 1;
									if(toiptr[j] != tooptr[j])
										FD_SET(datafd[j], &wset);

									filecnt[j]++;
									progresssize = 0;
								}
							}
						}
					}
				}
			}
		}
	}

	return;
}
