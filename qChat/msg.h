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
#include <arpa/inet.h>

#define BUFFLEN		2048

typedef enum {
	CHAT_AUTH_REQ,
	CHAT_AUTH_REP,
	CHAT_DATA,
	CHAT_USER_SUMMARY,
    CHAT_REQ_USER_SUMMARY
} chat_msg;

typedef struct {
	int type;
	int dlen;
} chat_header;

typedef enum {AUTH_DEN, AUTH_SUCCESS} auth_res;

typedef struct {
} chat_auth_req;

typedef struct {
	int res_type;
} chat_auth_rep;

typedef struct {
	int nicklen;
	char *nick;
	int sock;
	unsigned long int ptr;
	void *next;
} usr_info;
