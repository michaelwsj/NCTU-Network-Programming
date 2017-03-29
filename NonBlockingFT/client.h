#include "header.h"

#define MAXLINE 1024
#define DATALSTNPORT 2222

#define SA struct sockaddr
#define max(a, b) ({__typeof__ (a) _a; __typeof__ (b) _b; _a > _b ? _a : _b;})

extern int blockingwrite(int fd, char *str, int len);
extern char* gf_time(void);
extern void Sleep(int slptime);
extern void clientfunc(int sockfd, const char *servip);
