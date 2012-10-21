/*
 * Copyright 2011-2013	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <netinet/in.h>
#include <pthread.h>
#include "msg.h"

#define MAX_CONN	5 

static struct usr_info *usrs = NULL;
static char server[] = "chat_server";

void frw_msg(int sock, char *msg, int msglen)
{
	struct usr_info *u = usrs;
	while (u) {
		if (u->sock != sock)
			snd_msg(msg, msglen, u->sock);
		u = u->next;
	}
}

void remove_user_info(int sock)
{
	struct usr_info *u2, *u1 = usrs;
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

void *client_thread(void *t)
{
	struct usr_info *info = (struct usr_info *)t;
	char *msg = (char *) malloc(BUFFLEN);
	struct chat_header *ch;
	struct chat_data *data;

	while (1) {
		int len;
		memset(msg, 0, BUFFLEN);
		if ((len = recv(info->sock, msg, BUFFLEN - 1, 0)) <= 0)
			break;
		else {
			ch = (struct chat_header *) msg;
			switch (ch->type) {
			case CHAT_DATA:
				data = (struct chat_data *)(ch + 1);
#ifdef DEBUG
				printf("%s: %s\n", ch->nick, data->data);
#endif
				frw_msg(info->sock, msg, BUFFLEN);
				break;
			default:
				break;
			}
		}
	}
	memset(msg, 0, BUFFLEN);
	make_chat_header(msg, CHAT_USER_DISC, info->nick, NICKLEN);
	frw_msg(info->sock, msg, BUFFLEN);

	remove_user_info(info->sock);
	pthread_exit(NULL);
}

int client_auth(int sock)
{
	char *buff = (char *) malloc(BUFFLEN);
	memset(buff, 0, BUFFLEN);
	if (read(sock, buff, BUFFLEN - 1) <= 0) {
#ifdef DEBUG
		printf("%s: error receiving data\n", __func__);
#endif
		return -1;
	}
	struct chat_header *ch = (struct chat_header *) buff;
	if (ch->type == CHAT_AUTH_REQ) {
		/*
		 * XXX: open authentication for the moment
		 */
		struct usr_info *uinfo = (struct usr_info *)
			malloc(sizeof(struct usr_info));
		memset(uinfo, 0, sizeof(struct usr_info));
		if (!uinfo)
			return -1;
		uinfo->sock = sock;
		memcpy(uinfo->nick, ch->nick, NICKLEN);
		if (pthread_create(&uinfo->ptr, NULL, client_thread,
				   (void *)uinfo) < 0) {
#ifdef DEBUG
			printf("error creating thread\n");
#endif
			free(uinfo);
			return -1;
		}
		uinfo->next = usrs;
		usrs = uinfo;
		buff = (char *) malloc(BUFFLEN);
		memset(buff, 0, BUFFLEN);
		make_chat_header(buff, CHAT_AUTH_REP, server, strlen(server));
		make_auth_rep(buff, AUTH_SUCCESS);
		if (snd_msg(buff, BUFFLEN, sock) < 0)
			return -1;
#ifdef DEBUG
		printf("authentication successful\n");
#endif
		memset(buff, 0, BUFFLEN);
		make_chat_header(buff, CHAT_USET_CONN, ch->nick, NICKLEN);
		frw_msg(sock, buff, BUFFLEN);
		return 0;
	}
	return -1;
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
#ifdef DEBUG
		printf("error creating the socket\n");
#endif
		return -1;
	}
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval,
		       sizeof (optval)) < 0) {
#ifdef DEBUG
		printf("error setting socket option\n");
#endif
		return -1;
	}
	if (bind(sock, (const struct sockaddr*)&sa, sizeof(sa)) < 0) {
#ifdef DEBUG
		printf("error binding the socket\n");
#endif
		return -1;
	}
	if (listen(sock, MAX_CONN)) {
#ifdef DEBUG
		printf("error in listening the socket\n");
#endif
		return -1;
	}
	while (1) {
		int cs = accept(sock, NULL, NULL);
		if (cs < 0) {
#ifdef DEBUG
			printf("error accepting connection\n");
#endif
			return -1;
		}
		if (client_auth(cs) < 0) {
#ifdef DEBUG
			printf("authentication failed\n");
#endif
			close(cs);
			continue;
		}
	}
	close(sock);
	return 0;
}
