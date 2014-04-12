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
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include "msg.h"

#define MAX_CONN	5 

static usr_info *usrs = NULL;
static int un_depth = 0;
static int udepth = 0;
static char server[] = "chat_server";
static int slen;
static const struct timespec timeout = {5, 0};

static volatile int exit_req;

static void frw_msg(int sock, char *msg, int msglen)
{
	usr_info *u = usrs;

	while (u) {
		if (u->sock != sock)
			snd_msg(msg, msglen, u->sock);
		u = u->next;
	}
}

static void remove_user_info(int sock)
{
	usr_info *u2, *u1 = usrs;

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

static void signal_hl(int sig, siginfo_t *siginfo, void *context)
{
#ifdef DEBUG
	printf ("signal %d received from pid: %ld, uid: %ld\n",
		sig, (long)siginfo->si_pid, (long)siginfo->si_uid);
#endif
	exit_req = 1;
}

static void *client_thread(void *t)
{
	usr_info *info = (usr_info *)t;
	char msg[BUFFLEN];
	int data_len;
	int user_sum_len;
	fd_set socks;

	while (!exit_req) {
		int len, err;

		memset(msg, 0, BUFFLEN);
		FD_ZERO(&socks);
		FD_SET(info->sock, &socks);
		err = pselect(info->sock + 1, &socks, NULL, NULL, &timeout,
			      NULL);
		if (err <= 0)
			continue;

		if (FD_ISSET(info->sock, &socks)) {
			if ((len = recv(info->sock, msg, BUFFLEN - 1, 0)) <= 0)
				break;
			else {
				chat_header *ch = (chat_header *)msg;
#ifdef DEBUG
				char *sinfo = (char *)(msg + sizeof(chat_header));
				int nicklen = ntohl(*(int *)(sinfo));
				char nick[nicklen + 1];
				memset((void *)nick, 0, nicklen + 1);
				strncpy(nick, (char *)(sinfo + 4), nicklen);
#endif
				switch (ntohl(ch->type)) {
				case CHAT_DATA: {
#ifdef DEBUG
					char *dptr;
					int datalen = ntohl(ch->dlen) -
						      4 - nicklen;
					char data[datalen + 1];
					dptr = (char *)(msg + sizeof(chat_header)
							+ 4 + nicklen);
					memset((void *)data, 0, datalen + 1);
					strncpy(data, dptr, datalen);
					printf("%s: %s\n", nick, data);
#endif
					frw_msg(info->sock, msg, len);
					break;
				}
				case CHAT_REQ_USER_SUMMARY: {
#ifdef DEBUG
					printf("%s: USER_SUMMARY Request\n",
					       nick);
#endif
					data_len = 4 + slen + un_depth +
						   4 * udepth;
					user_sum_len = sizeof(chat_header) +
						       data_len;
					char umsg[user_sum_len];
					memset(umsg, 0, user_sum_len);
					make_chat_header(umsg, CHAT_USER_SUMMARY,
							 data_len);
					make_nick_info(umsg, server, slen);
					make_chat_users_summary(umsg, slen, usrs);
					snd_msg(umsg, user_sum_len, info->sock);
					break;
				}
				default:
					break;
				}
			}
		}
	}
#ifdef DEBUG
	printf("%s is disconnected!!\n", info->nick);
#endif
	remove_user_info(info->sock);
	data_len = 4 + slen + un_depth + 4 * udepth;
	user_sum_len = sizeof(chat_header) + data_len;
	char umsg[user_sum_len];
	memset(umsg, 0, user_sum_len);
	make_chat_header(umsg, CHAT_USER_SUMMARY, data_len);
	make_nick_info(umsg, server, slen);
	make_chat_users_summary(umsg, slen, usrs);
	frw_msg(-1, umsg, user_sum_len);

	pthread_exit(NULL);
}

static int client_auth(int sock)
{
	int err;
	fd_set socks;
	char buff[BUFFLEN];

	FD_ZERO(&socks);
	FD_SET(sock, &socks);
	memset(buff, 0, BUFFLEN);

	err = pselect(sock + 1, &socks, NULL, NULL, &timeout, NULL);
	if (err <= 0)
		return -1;

	if (FD_ISSET(sock, &socks)) {
		if (read(sock, buff, BUFFLEN - 1) <= 0) {
#ifdef DEBUG
			printf("%s: error receiving data\n", __func__);
#endif
			return -1;
		}
		chat_header *ch = (chat_header *)buff;
		if (ntohl(ch->type) == CHAT_AUTH_REQ) {
			int user_sum_len, nicklen, data_len, rep_len;
			usr_info *uinfo;

			nicklen = ntohl(*(int *)(buff + sizeof(chat_header)));
			data_len = 4 + slen + sizeof(chat_auth_rep);
			rep_len = sizeof(chat_header) + data_len;
			/*
			 * XXX: open authentication for the moment
			 */
			uinfo = (usr_info *)malloc(sizeof(usr_info));
			memset(uinfo, 0, sizeof(usr_info));
			if (!uinfo)
				return -1;
			uinfo->sock = sock;
			uinfo->nicklen = nicklen;
			uinfo->nick = malloc(uinfo->nicklen + 1);
			memset((void *)uinfo->nick, 0, uinfo->nicklen + 1);
			strncpy(uinfo->nick, buff + sizeof(chat_header) + 4,
				uinfo->nicklen);
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
			char rrep[rep_len];
			memset(rrep, 0, rep_len);
			make_chat_header(rrep, CHAT_AUTH_REP, data_len);
			make_nick_info(rrep, server, slen);
			make_auth_rep(rrep, slen, AUTH_SUCCESS);
			if (snd_msg(rrep, rep_len, sock) < 0)
				return -1;
			data_len = 4 + slen + un_depth + 4 * udepth;
			user_sum_len = sizeof(chat_header) + data_len;
			char umsg[user_sum_len];
			memset(umsg, 0, user_sum_len);
			make_chat_header(umsg, CHAT_USER_SUMMARY, data_len);
			make_nick_info(umsg, server, slen);
			make_chat_users_summary(umsg, slen, usrs);
			frw_msg(-1, umsg, user_sum_len);
#ifdef DEBUG
			printf("authentication successful\n");
#endif
			return 0;
		}
	}
	return -1;
}

int main(int argc, char **argv)
{
	int port = 9999, sock, optval = 1;
	sigset_t mask, old_mask;
	struct sigaction sigact;
	struct sockaddr_in sa;
	fd_set socks;

	slen = strlen(server);

	if (argc ==  2)
		port = atof(argv[1]);
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	/* signal handling */
	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = &signal_hl;
	if (sigaction(SIGTERM, &sigact, NULL) < 0 ||
	    sigaction(SIGINT, &sigact, NULL) < 0) {
#ifdef DEBUG
		printf("error registering signal handler\n");
#endif
		return -1;
	}
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	if (sigprocmask(SIG_BLOCK, &mask, &old_mask) < 0) {
#ifdef DEBUG
		printf("error blocking signal\n");
#endif
		return -1;
	}

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

	while (!exit_req) {
		int res, cs;

		FD_ZERO(&socks);
		FD_SET(sock, &socks);
		res = pselect(sock + 1, &socks, NULL, NULL, &timeout,
			      &old_mask);
		if (res < 0)
			break;
		else if (res == 0)
			continue;

		if (FD_ISSET(sock, &socks) &&
		    (cs = accept(sock, NULL, NULL)) >= 0) {
			if (client_auth(cs) < 0) {
#ifdef DEBUG
				printf("authentication failed\n");
#endif
				close(cs);
			}
		}
	}
	pthread_exit(NULL);
	close(sock);
	return 0;
}
