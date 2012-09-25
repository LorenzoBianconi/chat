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
#include <pthread.h>
#include <sys/types.h>

#define BUFFLEN		1024
#define MAX_CONN	5 

struct usrInfo {
	char nick[BUFFLEN];
	int sock;
	pthread_t ptr;
	struct usrInfo *next;
};

struct usrInfo *usrs = NULL;

void frwdMsg(int sock, char *msg)
{
	struct usrInfo *u = usrs;
	while (u) {
		if (u->sock != sock) {
			send(u->sock, msg, strlen(msg), 0);
		}
		u = u->next;
	}
}

void rmThrInfo(int sock)
{
	struct usrInfo *u2, *u1 = usrs;
	while (u1) {
		if (u1->sock == sock) {
			if (u1 == usrs)
				usrs = u1->next;
			else
				u2->next = u1->next;
			close(u1->sock);
			free(u1);
			return;
		}
		u2 = u1;
		u1 = u1->next;
	}
}

int clientAuth(int sock, char *buff)
{
	memset(buff, 0, BUFFLEN);
	if (read(sock, buff, BUFFLEN - 1) <= 0) {
		printf("%s: error receiving data\n", __func__);
		return -1;
	}
	if (send(sock, "OK", 3, 0) < 0) {
		printf("%s: error sending data to the server\n", __func__);
		return -1;
	}
	return 0;

}

void *client_thread(void *t)
{
	struct usrInfo *info = (struct usrInfo *)t;
	char buff[BUFFLEN], msg[BUFFLEN];

	while (1) {
		memset(msg, 0, BUFFLEN);
		if (read(info->sock, msg, BUFFLEN - 1) <= 0)
			goto exit;
		else {
			memset(buff, 0, BUFFLEN);
			strncat(buff, info->nick, strlen(info->nick));
			strncat(buff, ": ", 3);
			strncat(buff, msg, strlen(msg));
			printf("%s", buff);
			frwdMsg(info->sock, buff);
		}
	}
exit:
	memset(buff, 0, BUFFLEN);
	strncat(buff, info->nick, strlen(info->nick));
	strncat(buff, " is disconnected\n", 17);
	frwdMsg(info->sock, buff);
	printf("%s", buff);

	rmThrInfo(info->sock);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int port = 9999, sock, optval = 1;
	struct sockaddr_in sa;

	if (argc ==  2)
		port = atof(argv[1]);
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		printf("error creating the socket\n", __func__);
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       &optval, sizeof (optval)) < 0) {
		printf("error setting socket option\n");
		return -1;
	}
	if (bind(sock, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
		printf("error binding the socket\n", __func__);
		return -1;
	}
	if (listen(sock, MAX_CONN)) {
		printf("error in listening the socket\n", __func__);
		return -1;
	}
	while (1) {
		char nick[BUFFLEN];
		struct usrInfo *uinfo;
		int cs = accept(sock, NULL, NULL);
		if (cs < 0) {
			printf("error accepting connection\n", __func__);
			return -1;
		}
		if (clientAuth(cs, nick) < 0) {
			printf("authentication failed\n", __func__);
			close(cs);
			continue;
		} else {
			char buff[BUFFLEN];
			memset(buff, 0, BUFFLEN);
			strncat(buff, nick, strlen(nick));
			strncat(buff, " is connected\n", 14);
			frwdMsg(cs, buff);
			printf("%s", buff);
		}
		uinfo = (struct usrInfo*) malloc(sizeof(struct usrInfo));
		if (!uinfo)
			return -1;
		uinfo->sock = cs;
		memcpy(uinfo->nick, nick, strlen(nick));
		if (pthread_create(&uinfo->ptr, NULL, client_thread,
				   (void *)uinfo) < 0) {
			printf("error creating thread\n");
			free(uinfo);
			return -1;
		}
		uinfo->next = usrs;
		usrs = uinfo;
	}
	close(sock);
	return 0;
}
