/* Reliable File Transfer with SIGALRM timeout method */
#include "sender.h"

pkg_t pkgbuf[WINDOWSIZE];
int sndptr, rcvptr;
unsigned int base, nextseq;

SA *_rcvaddr;
socklen_t _rcvlen;

int _sockfd;

void sig_alrm(int signo){
	retransmission();

	return;
}

void rdt_send(int sockfd, SA *rcvaddr, socklen_t rcvlen, char *filename){
	FILE *fptr;
	int filefd;

	int pkglen, datalen;
	char pkg[PKGSIZE];

	char ackfield[PKGSIZE];
	unsigned int acknum;
	int dupcnt;	// duplicate ACK count
	pkg_t *pkgptr;

	int eof;

	_sockfd = sockfd;
	_rcvaddr = rcvaddr;
	_rcvlen = rcvlen;

	Connect(_sockfd, _rcvaddr, _rcvlen);

	signal(SIGALRM, sig_alrm);

	/* start transferring file */
	if((fptr = fopen(filename, "r")) == NULL){
		fprintf(stderr, "Failed to open the file.\n");
		return;
	}

	filefd = fileno(fptr);

	base = 1;
	nextseq = 1;
	eof = 0;

	while(1){
		nextseq = base;
		sndptr = 0;

		/* read file and send */
		while(sndptr < WINDOWSIZE){
			memset(pkgbuf[sndptr].pkg, 0, PKGSIZE);
			if(eof == 0 && (datalen = read(filefd, pkgbuf[sndptr].pkg + 4, DATASIZE)) == 0){	// end of file
				close(filefd);
				fclose(fptr);
				eof = 1;
			}

			if(eof == 1){	// end of file
				memcpy(pkgbuf[sndptr].pkg, &nextseq, 4);
				strncpy(pkgbuf[sndptr].pkg + 4, "FIN", 3);
				datalen = 3;
}
			else{
				memcpy(pkgbuf[sndptr].pkg, &nextseq, 4);
			}

			pkgbuf[sndptr].pkg[datalen + 4] = '\0';
			pkgbuf[sndptr].len = datalen + 4;
			pkgbuf[sndptr].seqnum = nextseq;
/*
			{
				int seq;
				char *data;

				memcpy(&seq, pkgbuf[sndptr].pkg, 4);
				data = pkgbuf[sndptr].pkg + 4;

				printf("seq: %d\n", seq);
				printf("%s\n", data);
			}
*/
			sendto(_sockfd, pkgbuf[sndptr].pkg, pkgbuf[sndptr].len, 0, _rcvaddr, _rcvlen);
//			write(_sockfd, pkgbuf[sndptr].pkg, pkgbuf[sndptr].len);

			if(base == nextseq)
				alarm(TIMEOUT);

			if(eof != 1)
				nextseq++;
			sndptr++;
		}

		rcvptr = 0;
		dupcnt = 0;

		/* receive ACK or retransmit */
		while(rcvptr < WINDOWSIZE){
			pkglen = recvfrom(_sockfd, pkg, PKGSIZE, 0, _rcvaddr, &_rcvlen);
//			pkglen = read(_sockfd, pkg, PKGSIZE);
			pkg[pkglen] = '\0';

			strcpy(ackfield, strtok(pkg, " "));
			acknum = atoi(strtok(NULL, " "));

			if(strcmp(ackfield, "ACK") != 0)
				continue;

			if(eof == 1 && acknum >= nextseq - 1){	// end of file
				break;
			}

			if(pkgbuf[rcvptr].seqnum > acknum){	// duplicate ACK
				dupcnt++;

				if(dupcnt == 3)	// fast retransmit
					retransmission();
			}
			else{
				rcvptr = acknum - base + 1;	// cumulative ACK

				dupcnt = 0;
			}
		}

		base += WINDOWSIZE;
		alarm(0);

		/* send at least five multiple FIN to ensure receiver to receive */
		if(eof == 1){
			break;
		}
	}
}

void retransmission(void){
	for(int i = rcvptr; i < WINDOWSIZE; i++)
		sendto(_sockfd, pkgbuf[i].pkg, pkgbuf[i].len, 0, _rcvaddr, _rcvlen);
//		write(_sockfd, pkgbuf[i].pkg, pkgbuf[i].len);
	alarm(TIMEOUT);	// restart timer

	return;
}
