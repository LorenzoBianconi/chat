/*
 * Copyright 2011-2012	Lorenzo Bianconi <me@lorenzobianconi.net>
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
#include <sys/socket.h>
#include <sys/types.h>

#define BUFFLEN		1024
#define NICKLEN		128
#define DATALEN		(BUFFLEN - sizeof(struct chat_header))

enum chat_msg {CHAT_AUTH_REQ, CHAT_AUTH_REP, CHAT_DATA};

struct chat_header {
	enum chat_msg type;
	char nick[NICKLEN];
};

enum auth_res {AUTH_DEN, AUTH_SUCCESS};

struct chat_auth_req {
};

struct chat_auth_rep {
	enum auth_res res_type;
};

struct chat_data {
	char data[DATALEN];
};

struct usr_info {
	char nick[NICKLEN];
	int sock;
	pthread_t ptr;
	struct usr_info *next;
};

int snd_msg(char *, int, int);
void make_chat_header(char *, enum chat_msg, char *, int);
void make_chat_data(char *, char *, int);
void make_auth_req(char *);
void make_auth_rep(char *, enum auth_res );
