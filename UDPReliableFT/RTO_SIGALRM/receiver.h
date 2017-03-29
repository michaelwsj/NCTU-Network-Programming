#include "header.h"

#define MAXLINE 1024
#define DATASIZE 1024
#define HEADERSIZE 10 + 1	// seqnum (max: 4294967295) + space
#define PKGSIZE DATASIZE + HEADERSIZE
#define WINDOWSIZE 1024
#define TIMEOUT 3

#define SA struct sockaddr

struct pkg_t{
	char data[DATASIZE];
	int len;
	unsigned int seqnum;
};
typedef struct pkg_t pkg_t;

extern int rcvmrk[WINDOWSIZE];
extern pkg_t pkgbuf[WINDOWSIZE];

extern void rdt_receive(int sockfd, char *filename);
extern void bufferclear(int seqnum);
