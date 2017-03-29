CFLAG= -g -Wall
CC=gcc

all:	server client

server:	server.o wrappers.o serverfunc.o
	gcc -o server server.o wrappers.o serverfunc.o 

client:	client.o wrappers.o clientfunc.o
	gcc -o client client.o wrappers.o clientfunc.o

server.o:	server.c server.h header.h
	gcc -c -o server.o server.c

serverfunc.o:	serverfunc.c server.h header.h
	gcc -c -o serverfunc.o serverfunc.c

client.o:	client.c client.h header.h
	gcc -c -o client.o client.c

clientfunc.o: clientfunc.c client.h header.h
	gcc -c -o clientfunc.o clientfunc.c

wrappers.o:	wrappers.c header.h
	gcc -c -o wrappers.o wrappers.c

clean:
	rm -f server client *.o
