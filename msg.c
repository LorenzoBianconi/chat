/*
 * Copyright 2011-2013	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "msg.h"
#include <sys/socket.h>
#include <sys/types.h>

int snd_msg(char *msg, int msglen, int sock)
{
	msg[msglen -1] = '\n';
	if (send(sock, msg, msglen, 0) < 0) {
#ifdef DEBUG
		printf("error sending buffer\n");
#endif
		return -1;
	}
	return 0;
}

void make_chat_header(char *msg, enum chat_msg type, int msglen,
		      char *nick, int nicklen)
{
	struct chat_header *ch = (struct chat_header *) msg;
	ch->type = type;
	ch->len = sizeof(struct chat_header) + msglen;
	memcpy(ch->nick, nick, nicklen);
}

void make_chat_data(char *msg, char *data, int datalen)
{
	struct chat_data *d = (struct chat_data *)
		(msg + sizeof(struct chat_header));
	memcpy(d->data, data, datalen);
}

void make_auth_rep(char *msg, enum auth_res res)
{
	struct chat_auth_rep *rep = (struct chat_auth_rep *)
		(msg + sizeof(struct chat_header));
	rep->res_type = res;
}

void make_auth_req(char *msg)
{
}

void make_chat_users_summary(char *msg, struct usr_info *users)
{
	struct chat_user_summary *usum = (struct chat_user_summary *) (msg +
			sizeof(struct chat_header));
	struct usr_info *tmp_user = users;
	while (tmp_user) {
		memcpy(usum->nick, tmp_user->nick, NICKLEN);
		tmp_user = tmp_user->next;
		usum += 1;
	}
}
