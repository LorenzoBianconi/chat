CC = gcc
LIBS = -lpthread

.c.o:
	$(CC) -c $*.c

all		: server client
server		: server.o
	$(CC) server.o $(LIBS) -o server
client		: client.o
	$(CC) client.o $(LIBS) -o client
server.o	: server.c
client.o	: client.c

clean:
	rm -f  *.o server client
