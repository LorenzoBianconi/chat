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
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#define MAX_CONN	5 

static struct usr_info *usrs = NULL;
static int un_depth = 0;
static int udepth = 0;
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
			un_depth -= u1->nicklen;
			udepth--;
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
	int data_len;
	int user_sum_len;

	while (1) {
		int len;

		memset(msg, 0, BUFFLEN);
		if ((len = recv(info->sock, msg, BUFFLEN - 1, 0)) <= 0)
			break;
		else {
			struct chat_header *ch = (struct chat_header *) msg;

			switch (ntohl(ch->type)) {
			case CHAT_DATA: {
#ifdef DEBUG
				int nicklen = ntohl(*(int *)(msg + sizeof(struct chat_header)));
				int datalen = ntohl(ch->dlen) - (4 + nicklen);
				char nick[nicklen + 1];
				char data[datalen + 1];

				strncpy(nick, (char *)(msg + sizeof(struct chat_header) + 4),
				       nicklen);
				strncpy(data, (msg + sizeof(struct chat_header) + 4 + nicklen),
				       datalen);
				printf("%s: %s\n", nick, data);
#endif
				frw_msg(info->sock, msg, len);
				break;
			}
			default:
				break;
			}
		}
	}
#ifdef DEBUG
	printf("%s is disconnected!!\n", info->nick);
#endif
	remove_user_info(info->sock);
	data_len = 4 + strlen(server) + un_depth + 4 * udepth;
	user_sum_len = sizeof(struct chat_header) + data_len;
	msg = (char *) malloc(user_sum_len);
	memset(msg, 0, user_sum_len);
	make_chat_header(msg, CHAT_USER_SUMMARY, data_len);
	make_nick_info(msg, server, strlen(server));
	make_chat_users_summary(msg, strlen(server), usrs);
	frw_msg(-1, msg, user_sum_len);

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
	if (ntohl(ch->type) == CHAT_AUTH_REQ) {
		int user_sum_len;
		int nicklen = ntohl(*(int *)(buff + sizeof(struct chat_header)));
		int data_len = 4 + strlen(server) + sizeof(struct chat_auth_rep);
		int rep_len = sizeof(struct chat_header) + data_len;
		/*
		 * XXX: open authentication for the moment
		 */
		struct usr_info *uinfo = (struct usr_info *) malloc(sizeof(struct usr_info));

		memset(uinfo, 0, sizeof(struct usr_info));
		if (!uinfo)
			return -1;
		uinfo->sock = sock;
		uinfo->nicklen = nicklen;
		uinfo->nick = malloc(uinfo->nicklen + 1);
		strncpy(uinfo->nick, buff + sizeof(struct chat_header) + 4, uinfo->nicklen);
		if (pthread_create(&uinfo->ptr, NULL, client_thread,
				   (void *)uinfo) < 0) {
#ifdef DEBUG
			printf("error creating thread\n");
#endif
			free(uinfo);
			return -1;
		}
		uinfo->next = usrs;
		un_depth += uinfo->nicklen;
		udepth++;
		usrs = uinfo;
#ifdef DEBUG
		printf("%s: %s is connected!!\n", __func__, uinfo->nick);
#endif
		buff = (char *) malloc(rep_len);
		memset(buff, 0, rep_len);
		make_chat_header(buff, CHAT_AUTH_REP, data_len);
		make_nick_info(buff, server, strlen(server));
		make_auth_rep(buff, strlen(server), AUTH_SUCCESS);
		if (snd_msg(buff, rep_len, sock) < 0)
			return -1;
		data_len = 4 + strlen(server) + un_depth + 4 * udepth;
		user_sum_len = sizeof(struct chat_header) + data_len;
		buff = (char *) malloc(user_sum_len);
		memset(buff, 0, user_sum_len);
		make_chat_header(buff, CHAT_USER_SUMMARY, data_len);
		make_nick_info(buff, server, strlen(server));
		make_chat_users_summary(buff, strlen(server), usrs);
		frw_msg(-1, buff, user_sum_len);
#ifdef DEBUG
		printf("authentication successful\n");
#endif
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
