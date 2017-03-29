/* Reliable File Transfer with SIGALRM timeout method */
#include "receiver.h"

int rcvmrk[WINDOWSIZE];	// mark buffer as received
pkg_t pkgbuf[WINDOWSIZE];	// out-of-order pkg buffer

void rdt_receive(int sockfd, char *filename){
	SA sndaddr;
	socklen_t sndlen;

	FILE *fptr;
	int filefd;

	int pkglen;
	char pkg[PKGSIZE];

	unsigned int seqnum;
	char seqnum_str[DATASIZE];
//	char data[DATASIZE];
	char *data;

	unsigned int expectseq;

	int i, maxi;

	int eof;

	if((fptr = fopen(filename, "w")) == NULL){
		fprintf(stderr, "Failed to create the file.\n");
		return;
	}

	filefd = fileno(fptr);

	expectseq = 1;

	for(int i = 0; i < WINDOWSIZE; i++)
		rcvmrk[i] = 0;

	maxi = -1;

	eof = 0;

	while((pkglen = recvfrom(sockfd, pkg, PKGSIZE, 0, &sndaddr, &sndlen)) != 0){
		pkg[pkglen] = '\0';	

		/* extract pkg */
		memcpy(&seqnum, pkg, 4);
		data = pkg + 4;

		if(seqnum == expectseq){	// in-order pkg
			if(strcmp(data, "FIN") == 0)
				eof = 1;
			else
				write(filefd, data, pkglen - 4);
			expectseq++;

			/* send ACK */
			sprintf(pkg, "ACK %u", expectseq - 1);
			sendto(sockfd, pkg, strlen(pkg), 0, &sndaddr, sndlen);

			while(1){
				int pre_i = -1, check = 1;
				for(i = 0; i <= maxi; i++){
					if(rcvmrk[i] == 1 && pkgbuf[i].seqnum == expectseq){
						if(strcmp(pkgbuf[i].data, "FIN") == 0){
							eof = 1;
							printf("receive FIN\n");
						}
						else
							write(filefd, pkgbuf[i].data, pkgbuf[i].len);
						expectseq++;

						/* send ACK */
						sprintf(pkg, "ACK %u", expectseq - 1);
						sendto(sockfd, pkg, strlen(pkg), 0, &sndaddr, sndlen);

						rcvmrk[i] = 0;
						check = 0;
					}
					else if(rcvmrk[i] == 1)
						pre_i = i;
				}
				maxi = pre_i;

				if(check == 1)	// check if there is in-order pkg in buffer
					break;
			}
		}
		else if(seqnum > expectseq){	// out-of-order pkg
			for(i = 0; i < WINDOWSIZE; i++)
				if(rcvmrk[i] == 0)
					break;

			if(i == WINDOWSIZE){
//				fprintf(stderr, "Out-of-order buffer is full.\n");
				bufferclear(expectseq - 1);
			}

			if(i > maxi)
				maxi = i;

//			strcpy(pkgbuf[i].data, data);
			memcpy(pkgbuf[i].data, data, pkglen - 4);
			pkgbuf[i].data[pkglen - 4] = '\0';
			pkgbuf[i].len = pkglen - 4;
			pkgbuf[i].seqnum = seqnum;

			rcvmrk[i] = 1;

			/* send duplicate ACK */
			sprintf(pkg, "ACK %u", expectseq - 1);
			sendto(sockfd, pkg, strlen(pkg), 0, &sndaddr, sndlen);
		}
		else{	// duplicate pkg
			sprintf(pkg, "ACK %u", expectseq - 1);
			sendto(sockfd, pkg, strlen(pkg), 0, &sndaddr, sndlen);
		}

		if(eof == 1)
			break;

		memset(pkg, 0, PKGSIZE);
	}

	/* send at least five multiple eof ACK to ensure sender to receive */
	for(int i = 0; i < 5; i++)	
		sendto(sockfd, pkg, strlen(pkg), 0, &sndaddr, sndlen);

	close(filefd);
	fclose(fptr);
}

void bufferclear(int seqnum){
	for(int i = 0; i < WINDOWSIZE; i++){
		if(pkgbuf[i].seqnum <= seqnum)
			rcvmrk[i] = 0;
	}

	return;
}
