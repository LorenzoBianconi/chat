CC = gcc
LIBS = -lpthread
#DEFINES = -DDEBUG
#OPTIMIZE = -g -Wall
OPTIMIZE = -O3 -s -fomit-frame-pointer
CFLAGS   = $(DEFINES) $(OPTIMIZE)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

all		: server
server		: server.o msg.o
	$(CC) server.o msg.o $(LIBS) -o server
server.o	: msg.h msg.c server.c
clean:
	rm -f  *.o server
