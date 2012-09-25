/*
 * Copyright 2011-2012	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/types.h>

#define BUFFLEN		1024

int sndMsg(char *msg, int sock)
{
	if (send(sock, msg, strlen(msg), 0) < 0) {
		printf("error sending data to the server\n");
		return -1;
	}
	return 0;
}

int clientAuth(int sock, char *msg)
{
	char buff[BUFFLEN];
	memset(buff, 0, BUFFLEN);

	sndMsg(msg, sock);
	if (read(sock, buff, BUFFLEN - 1) <= 0) {
		printf("%s: error receiving data\n", __func__);
		return -1;
	}
	return strncmp(buff, "OK\n", strlen(buff));
}

void *read_thread(void *t)
{
	char buff[BUFFLEN];
	int *sock = (int *)t;

	while (1) {
		memset(buff, 0, BUFFLEN);
		if (read(*sock, buff, BUFFLEN - 1) <= 0)
			goto exit;
		else
			printf("%s", buff);
	}
exit:
	printf("%s: closing socket %d\n", __func__, *sock);
	close(*sock);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int sock, port = 9999;
	char *host, *nick;
	struct hostent *saddr;
	struct sockaddr_in sa;
	pthread_t ptr;

	if (argc < 3) {
		printf("usage: <host> <port> <user name>\n");
		return -1;
	}
	host = argv[1];
	saddr = gethostbyname(host);
	if (!saddr) {
		printf("server %s not found\n", host);
		return -1;
	}
	if (argc == 4) {
		port = atof(argv[2]);
		nick = argv[3];
	} else
		nick = argv[2];

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, saddr->h_addr, saddr->h_length);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("error creating the socket\n");
		return -1;
	}
	if (connect(sock, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
		printf("error connecting to the server\n");
		return -1;
	} else {
		if (clientAuth(sock, nick) != 0) {
			printf("authentication error\n");
			return -1;
		}
		if (pthread_create(&ptr, NULL, read_thread, (void *)&sock) < 0) {
			printf("error creating thread\n");
			return -1;
		}
	}

	while (1) {
		char msg[BUFFLEN];
		memset(msg, 0, BUFFLEN);
		fgets(msg, BUFFLEN, stdin);
		sndMsg(msg, sock);
	}
	return 0;
}
