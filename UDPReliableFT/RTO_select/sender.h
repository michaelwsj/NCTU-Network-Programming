#include "header.h"

#define MAXLINE 1024
#define LISTENQ 1024
#define DATASIZE 1024
#define HEADERSIZE 10 + 1	// seqnum (max: 4294967295) + space
#define PKGSIZE DATASIZE + HEADERSIZE
#define WINDOWSIZE 32
#define TIMEOUT 1

#define SA struct sockaddr
#define max(a, b) ({__typeof__ (a) _a; __typeof__ (b) _b; _a > _b ? _a : _b;})

struct pkg_t{
	char pkg[PKGSIZE];
	int len;
	unsigned int seqnum;
};
typedef struct pkg_t pkg_t;

extern pkg_t *basepkg, *toppkg;
extern unsigned int base, nextseq;
extern SA *_rcvaddr;
extern socklen_t _rcvlen;
extern int _sockfd;

extern int readable_timeo(int sockfd, int sec);
extern void rdt_send(int sockfd, SA *rcvaddr, socklen_t rcvlen, char *filename);
extern void retransmission(void);
