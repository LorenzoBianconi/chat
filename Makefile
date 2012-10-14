CC = gcc
LIBS = -lpthread
DEFINES = -DDEBUG
OPTIMIZE = -g -Wall
#OPTIMIZE = -O3 -s -fomit-frame-pointer
CFLAGS   = $(DEFINES) $(OPTIMIZE)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all		: server client
server		: server.o msg.o
	$(CC) server.o msg.o $(LIBS) -o server
client		: client.o msg.o
	$(CC) client.o msg.o $(LIBS) -o client
server.o	: msg.h msg.c server.c
client.o	: msg.h msg.c client.c

clean:
	rm -f  *.o server client
