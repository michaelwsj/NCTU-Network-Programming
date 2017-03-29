/* client function */
#include "client.h"

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

void Sleep(int slptime){
	int n, sec = 0;
	char str[MAXLINE];

	printf("Client starts to sleep\n");

	while(++sec <= slptime){
		sleep(1);

		sprintf(str, "Sleep %d\n", sec);
		if((n = blockingwrite(STDOUT_FILENO, str, strlen(str))) < 0)
			err_sys("write error on stdout");
	}

	printf("Client wakes up\n");

	return;
}

void clientfunc(int sockfd, const char *servip){
	int frfd, fwfd, datafd;
	int maxfdp1, val, exit;
	int stdineof, data_write, data_read, file_write, file_read;
	int n, nwritten;
	fd_set rset, wset;
	char to[MAXLINE], fr[MAXLINE], readline[MAXLINE], writeline[MAXLINE];
	char *toiptr, *tooptr, *friptr, *froptr;
	int dataport;
	struct sockaddr_in dataaddr;
	char progressfile[MAXLINE];
	int filesize, progresssize;
	struct stat filestat;
	char filename[MAXLINE];

	/* add O_NONBLOCK to file status */
	val = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

	val = fcntl(STDOUT_FILENO, F_GETFL, 0);
	fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

	/* initialize buffer pointer */
	toiptr = tooptr = to;
	friptr = froptr = fr;

	exit = 0;
	stdineof = 0;
	data_write = 0;
	data_read = 0;
	file_write = 0;
	file_read = 0;

	// data port initialize
	datafd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&dataaddr, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr. sin_port = htons(DATALSTNPORT);
	inet_pton(AF_INET, servip, &dataaddr.sin_addr);

	Connect(datafd, (SA *)&dataaddr, sizeof(dataaddr));

	val = fcntl(datafd, F_GETFL, 0);
	fcntl(datafd, F_SETFL, val | O_NONBLOCK);

	// maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO), sockfd) + 1;

	for( ; ; ){
		if(exit == 1 && data_write == 0 && file_write == 0)	// exit
			/*
			 * Check if both buffers' write sides are done.
			 * Alternative condition is to check if both buffers are empty.
		 	*/
			return;

		FD_ZERO(&rset);
		FD_ZERO(&wset);

		if(stdineof == 0){
			FD_SET(STDIN_FILENO, &rset);	// read from stdin
			maxfdp1 = STDIN_FILENO;
		}
		FD_SET(sockfd, &rset);	// read from socket
		maxfdp1 = (maxfdp1 < sockfd + 1) ? sockfd + 1 : maxfdp1;

		if(file_read == 1 && toiptr < &to[MAXLINE]){
			FD_SET(frfd, &rset);	// read from file
			maxfdp1 = (maxfdp1 < frfd + 1) ? frfd + 1 : maxfdp1;
		}
		if(data_write == 1 && tooptr != toiptr){
			FD_SET(datafd, &wset);	// write to data socket
			maxfdp1 = (maxfdp1 < datafd + 1) ? datafd + 1 : maxfdp1;
		}

		if(data_read == 1 && friptr < &fr[MAXLINE]){
			FD_SET(datafd, &rset);	// read from data socket
			maxfdp1 = (maxfdp1 < datafd + 1) ? datafd + 1 : maxfdp1;
		}
		if(file_write == 1 && froptr != friptr){
			FD_SET(fwfd, &wset);	// write to file
			maxfdp1 = (maxfdp1 < fwfd + 1) ? fwfd + 1 : maxfdp1;
		}

		Select(maxfdp1, &rset, &wset, NULL, 0);

		if(FD_ISSET(STDIN_FILENO, &rset) && data_write != 1){
			if((n = read(STDIN_FILENO, readline, MAXLINE)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("read error on stdin");
			}
			else if(n == 0){
				// fprintf(stderr, "%s: EOF on stdin\n", gf_time());
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
			}
			else{
				// fprintf(stderr, "%s: read %d bytes from stdin\n", gf_time(), n);

				char *cmd;
				readline[n] = '\0';
				cmd = strtok(readline, "/ \n");

				if(strcmp(cmd, "put") == 0){	// /put <filename>
					if(file_read == 1){
						/*
						 * Actually, dropbox allow multiple files to be put simultineously.
						 * To implement it, there should be multiple buffers to handle file transferring.
						 * Or a list recording files to be transferred.
						 * Simplify it in this code.
						 */
						// fprintf(stderr, "%s: still putting previous file\n", gf_time());
					}
					else{
						char *arg;
						arg = strtok(NULL, "/ \n");
					
						/* frfd initialize */
						if((frfd = open(arg, O_RDONLY)) < 0){
							fprintf(stderr, "%s: failed to open the file %s\n", gf_time(), arg);
							continue;
						}

						val = fcntl(frfd, F_GETFL, 0);
						fcntl(frfd, F_SETFL, val | O_NONBLOCK);

						file_read = 1;
						// if(maxfdp1 - 1 < frfd)
						// 	maxfdp1 = frfd + 1;
						if(toiptr < &to[MAXLINE])
							FD_SET(frfd, &rset);

						strcpy(progressfile, arg);
						if(stat(progressfile, &filestat) == -1)
							err_sys("stat error");
						filesize = filestat.st_size;

						/* write request to server */
						sprintf(writeline, "PUT %s %d", arg, filesize);
						if((n = blockingwrite(sockfd, writeline, strlen(writeline))) < 0)
							err_sys("write error on socket");

						data_write = 1;
						// if(maxfdp1 - 1 < datafd)
						// 	maxfdp1 = datafd + 1;
						if(toiptr != tooptr)
							FD_SET(datafd, &wset);

						sprintf(writeline, "Uploagind file: %s\nProgress:[                    ]\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", progressfile);
						if((n = blockingwrite(STDOUT_FILENO, writeline, strlen(writeline))) < 0)
							err_sys("write error on stdout");
						progresssize = 0;
					}
				}
				else if(strcmp(cmd, "sleep") == 0){	// /sleep <seconds>
					char *arg;
					int slptime;

					arg = strtok(NULL, "/ \n");
					slptime = atoi(arg);

					Sleep(slptime);
				}
				else if(strcmp(cmd, "exit") == 0){	// /exit
					// fprintf(stderr, "%s: EOF on stdin\n", gf_time());
					stdineof = 1;
					shutdown(sockfd, SHUT_WR);
				}
				else{	// error command
					fprintf(stderr, "%s: error command %s\n", gf_time(), cmd);
				}
			}
		}

		if(FD_ISSET(sockfd, &rset) && file_write != 1){
			if((n = read(sockfd, readline, MAXLINE)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("read error on socket");
			}
			else if(n == 0){
				// fprintf(stderr, "%s: EOF on socket\n", gf_time());
				if(stdineof == 1)
					exit = 1;	// normal termination
				else
					err_quit("error: server terminated prematurely");
			}
			else{
				// fprintf(stderr, "%s: read %d bytes from socket\n", gf_time(), n);

				readline[n] = '\0';

				printf("%send\n", readline);	// debug
				fflush(stdout);

				char *cmd, *arg;
				cmd = strtok(readline, " ");
				arg = strtok(NULL, " ");
				
				if(strcmp(cmd, "SYN") == 0){	// SYN <filename>
					char *arg2;
					arg2 = strtok(NULL, " ");
					strcpy(filename, arg);
					filesize = atoi(arg2);

					strcpy(progressfile, arg);

					fwfd = open(arg, O_WRONLY | O_CREAT | O_TRUNC, 0644);

					val = fcntl(fwfd, F_GETFL, 0);
					fcntl(fwfd, F_SETFL, val | O_NONBLOCK);

					file_write = 1;
					// if(maxfdp1 - 1 < fwfd)
					// 	maxfdp1 = fwfd + 1;
					if(friptr < &fr[MAXLINE])
						FD_SET(fwfd, &wset);

					data_read = 1;
					// if(maxfdp1 - 1 < datafd)
					// 	maxfdp1 = datafd + 1;
					if(friptr != froptr)
						FD_SET(datafd, &rset);

					sprintf(writeline, "Downloagind file: %s\nProgress:[                    ]\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", progressfile);
					if((n = blockingwrite(STDOUT_FILENO, writeline, strlen(writeline))) < 0)
						err_sys("write error on stdout");
					fflush(stdout);
					progresssize = 0;
				}
				else{	// error command
					// fprintf(stderr, "%s: error command %s\n", gf_time(), cmd);
				}
			}
		}

		if(FD_ISSET(frfd, &rset)){
			if((n = read(frfd, toiptr, &to[MAXLINE] - toiptr)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("read error on file");
			}
			else if(n == 0){
				// fprintf(stderr, "%s: EOF on file\n", gf_time());
				file_read = 0;
				close(frfd);
			}
			else{
				// fprintf(stderr, "%s: read %d bytes from file\n", gf_time(), n);
				toiptr += n;
				// FD_SET(datafd, &wset);
			}
		}

		if(FD_ISSET(datafd, &wset) && ((n = toiptr - tooptr) > 0)){
			if((nwritten = write(datafd, tooptr, n)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("write error to data socket");
			}
			else{
				int progressratio;
				progressratio = (progresssize + nwritten) * 20 / filesize - progresssize * 20 / filesize;
				progresssize += nwritten;

				n = 0;
				while(n++ < progressratio){
					printf("#");	// too lazy to use self-defined blockingwrite function
					fflush(stdout);
				}

				// fprintf(stderr, "%s: wrote %d bytes to data socket\n", gf_time(), nwritten);
				tooptr += nwritten;

				if(tooptr == toiptr){
					toiptr = tooptr = to;	// back to beginning of the buffer
					if(progresssize == filesize){
						data_write = 0;
						printf("]\nUpload %s complete!\n", progressfile);	// too lazy to use self-defined blockingwrite function
						fflush(stdout);
					}
				}
			}
		}

		if(FD_ISSET(datafd, &rset)){
			if((n = read(datafd, friptr, &fr[MAXLINE] - friptr)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("read error on data socket");
			}
			else if(n == 0){
				// fprintf(stderr, "%s: EOF on data socket\n", gf_time());
				data_read = 0;
			}
			else{
				// fprintf(stderr, "%s: read %d bytes from data socket\n", gf_time(), n);
				friptr += n;
				// FD_SET(fwfd, &wset);
			}
		}

		if(FD_ISSET(fwfd, &wset) && ((n = friptr - froptr) > 0)){
			if((nwritten = write(fwfd, froptr, n)) < 0){
				if(errno != EWOULDBLOCK)
					err_sys("write error to file");
			}
			else{
				int progressratio;
				progressratio = (progresssize + nwritten) * 20 / filesize - progresssize * 20 / filesize;
				progresssize += nwritten;

				n = 0;
				while(n++ < progressratio){
					printf("#");	// too lazy to use self-defined blockingwrite function
					fflush(stdout);
				}

				// fprintf(stderr, "%s: wrote %d bytes to file\n", gf_time(), nwritten);
				froptr += nwritten;
				if(froptr == friptr){
					friptr = froptr = fr;	// back to beginning of the buffer
					if(progresssize == filesize){
						data_read = 0;
						file_write = 0;
						close(fwfd);	// end of writing file
						printf("]\nDownload %s complete!\n", progressfile);	// too lazy to use self-defined blockingwrite function
						fflush(stdout);

						// sprintf(writeline, "SYN-DONE %s", filename);
						// if((n = blockingwrite(sockfd, writeline, strlen(writeline))) < 0)
						// 	err_sys("write error on socket");
					}
				}
			}
		}
	}

	return;
}
