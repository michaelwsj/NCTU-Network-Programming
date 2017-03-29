#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXLINE 100

#define SA struct sockaddr

#define max(a, b) ({__typeof__ (a) _a; __typeof__ (b) _b; _a > _b ? _a : _b;})
