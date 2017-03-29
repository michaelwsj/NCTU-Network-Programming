all:	sender receiver

sender:	sender.o wrappers.o rdt_send.o
	gcc -o sender sender.o wrappers.o rdt_send.o 

receiver:	receiver.o wrappers.o rdt_receive.o
	gcc -o receiver receiver.o wrappers.o rdt_receive.o

sender.o:	sender.c sender.h header.h
	gcc -c -o sender.o sender.c

rdt_send.o:	rdt_send.c sender.h header.h
	gcc -c -o rdt_send.o rdt_send.c

receiver.o:	receiver.c receiver.h header.h
	gcc -c -o receiver.o receiver.c

rdt_receive.o: rdt_receive.c receiver.h header.h
	gcc -c -o rdt_receive.o rdt_receive.c

wrappers.o:	wrappers.c header.h
	gcc -c -o wrappers.o wrappers.c

clean:
	rm -f sender receiver *.o
