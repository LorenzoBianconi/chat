/*
 * Copyright 2011-2012	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "msg.h"

int client_auth(int sock, char *nick)
{
	int len = sizeof(struct chat_header) + sizeof(struct chat_auth_req) + 1;
	char *buff = (char *) malloc(len);
	memset(buff, 0, len);
	/*
	 * XXX: open authentication for the moment
	 */
	make_chat_header(buff, CHAT_AUTH_REQ, nick, NICKLEN);
	make_auth_req(buff);
	snd_msg(buff, len, sock);
	
	buff = (char *) malloc(BUFFLEN);
	memset(buff, 0, BUFFLEN);
	if (read(sock, buff, BUFFLEN - 1) <= 0) {
#ifdef DEBUG
		printf("%s: error receiving data\n", __func__);
#endif
		return -1;
	}
	struct chat_header *ch = (struct chat_header *) buff;
	if (ch->type == CHAT_AUTH_REP) {
		struct chat_auth_rep * arep = (struct chat_auth_rep *)(ch + 1);
		return arep->res_type == AUTH_SUCCESS;
	}
	return -1;
}

void *read_thread(void *t)
{
	char *buff;
	int *sock = (int *)t;
	struct chat_header *ch;
	struct chat_data *data;

	while (1) {
		buff = (char *) malloc(BUFFLEN);
		memset(buff, 0, BUFFLEN);
		if (read(*sock, buff, BUFFLEN - 1) <= 0) {
#ifdef DEBUG
			printf("error receiving data\n");
#endif
			goto exit;
		} else {
			ch = (struct chat_header *) buff;
			switch (ch->type) {
			case CHAT_DATA:
				data = (struct chat_data *)(ch + 1);
				printf("%s: %s", ch->nick, data->data);
				break;
			default:
				break;
			}
			printf("%s", buff);
		}
	}
exit:
	close(*sock);
#ifdef DEBUG
	printf("terminating receiving thread\n");
#endif
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int sock, port = 9999;
	char *host, *nick, *msg, *buff;
	struct hostent *saddr;
	struct sockaddr_in sa;
	pthread_t ptr;

	if (argc < 3) {
		printf("usage: <host> [port] <user name>\n");
		return -1;
	}
	host = argv[1];
	saddr = gethostbyname(host);
	if (!saddr) {
#ifdef DEBUG
		printf("server %s not found\n", host);
#endif
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
#ifdef DEBUG
		printf("error creating the socket\n");
#endif
		return -1;
	}
	if (connect(sock, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
#ifdef DEBUG
		printf("error connecting to the server\n");
#endif
		return -1;
	} else {
		if (client_auth(sock, nick) < 0) {
#ifdef DEBUG
			printf("authentication error\n");
#endif
			return -1;
		} else {
#ifdef DEBUG
			printf("authentication successful\n");
#endif
		}
		if (pthread_create(&ptr, NULL, read_thread,
				   (void *)&sock) < 0) {
#ifdef DEBUG
			printf("error creating thread\n");
#endif
			return -1;
		}
	}
	msg = malloc(BUFFLEN);
	buff = malloc(DATALEN);
	while (1) {
		memset(msg, 0, BUFFLEN);
		memset(buff, 0, DATALEN);
		make_chat_header(msg, CHAT_DATA, nick, strlen(nick));
		if (fgets(buff, DATALEN, stdin) == NULL)
			continue;
		make_chat_data(msg, buff, DATALEN);
		snd_msg(msg, BUFFLEN, sock);
	}
#ifdef DEBUG
	printf("closing connection\n");
#endif
	close(sock);
	return 0;
}
