/*
 * Copyright 2011-2012	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "msg.h"

int snd_msg(char *msg, int msglen, int sock)
{
	msg[msglen] = '\n';
	if (send(sock, msg, msglen, 0) < 0) {
#ifdef DEBUG
		printf("error sending buffer\n");
#endif
		return -1;
	}
	return 0;
}

void make_chat_header(char *msg, enum chat_msg type,
		      char *nick, int nicklen)
{
	struct chat_header *ch = (struct chat_header *) msg;
	ch->type = type;
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
