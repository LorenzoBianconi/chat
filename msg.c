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
	if (send(sock, msg, msglen, 0) < 0) {
#ifdef DEBUG
		printf("error sending buffer\n");
#endif
		return -1;
	}
	return 0;
}

void make_chat_header(char *msg, chat_msg type, int dlen)
{
	chat_header *ch = (chat_header *) msg;

	ch->type = htonl(type);
	ch->dlen = htonl(dlen);
}

void make_nick_info(char *msg, char *nick, int nicklen)
{
	*((int *) (msg + sizeof(chat_header))) = htonl(nicklen);
	memcpy(msg + sizeof(chat_header) + 4, nick, nicklen);
}

void make_chat_data(char *msg, char *data, int nicklen, int datalen)
{
	memcpy(msg + sizeof(chat_header) + 4 + nicklen, data, datalen);
}

void make_auth_rep(char *msg, int nicklen, auth_res res)
{
	chat_auth_rep *rep = (chat_auth_rep *)(msg + sizeof(chat_header) +
					       4 + nicklen);
	rep->res_type = htonl(res);
}

void make_auth_req(char *msg)
{
}

void make_chat_users_summary(char *msg, int nicklen, usr_info *users)
{
	usr_info *tmp_user = users;
	char *usum = (char *)(msg + sizeof(chat_header) + 4 + nicklen);

	while (tmp_user) {
		*((int *) usum) = htonl(tmp_user->nicklen);
		usum += 4;
		memcpy(usum, tmp_user->nick, tmp_user->nicklen);
		usum += tmp_user->nicklen;
		tmp_user = tmp_user->next;
	}
}
