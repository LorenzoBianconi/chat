/*
 * Copyright 2011-2013	Lorenzo Bianconi <me@lorenzobianconi.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFLEN		2048

enum chat_msg {
	CHAT_AUTH_REQ,
	CHAT_AUTH_REP,
	CHAT_DATA,
	CHAT_USER_SUMMARY
};

struct chat_header {
	int type;
	int dlen;
};

enum auth_res {AUTH_DEN, AUTH_SUCCESS};

struct chat_auth_req {
};

struct chat_auth_rep {
	int res_type;
};

struct usr_info {
	int nicklen;
	char *nick;
	int sock;
	unsigned long int ptr;
	struct usr_info *next;
};

int snd_msg(char *, int, int);
void make_nick_info(char *, char *, int);
void make_chat_header(char *, enum chat_msg, int);
void make_chat_data(char *, char *, int, int);
void make_auth_req(char *);
void make_auth_rep(char *, int, enum auth_res);
void make_chat_users_summary(char *, int, struct usr_info *);
