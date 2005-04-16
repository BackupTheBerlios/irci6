
/* $Id: c6.h,v 1.1 2005/04/16 09:23:22 benkj Exp $ */

/*
 * - irC6 -
 *
 * Copyright (c) 2003
 *	Leonardo Banchi		<benkj@antifork.org>.  
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * 
 */

#ifndef __c6_defs__
#define __c6_defs__

#include "c6/func.h"

SIMPLEQ_HEAD(proom_head, proom_entry);

typedef struct {
	int fd;
	char server_key[8];
	char reordered_key[8];
	char md5_key[16];

	char buf[65536];
	uint16_t old_len;

	uint16_t srv_id;
	uint16_t clt_id;

	uint16_t style;
	char *away;
	uint8_t search_id[256];

	struct proom_head proom;
	
	struct in_addr ip;
	struct sockaddr_in srv_addr;
	struct event ev;
} c6_t;

struct proom_entry {
	SIMPLEQ_ENTRY(proom_entry) entry;
	char *room;
};

struct values {
	int id;
	int val;
};

struct searchui {
	char *name; 
	size_t elms;
	const struct values *v;
};

struct userinfos {
	const char *name;
	size_t entries;
	const char * const *info;
};

extern const struct userinfos c6_userinfo[];
extern const size_t c6_userinfo_elms;

struct c6_pkt_head {
	uint8_t type;
	uint8_t cmd;
	uint16_t id;
	uint16_t len;
} __attribute__ ((packed));

#define C6(_c)	((c6_t *)(_c)->proto_mod)

#define C6_STATUS_ONLINE	0x40
#define C6_STATUS_CANCONF	0x20
#define C6_STATUS_BUSY		0x10
#define C6_STATUS_NETFRIENDS	0x08
#define C6_STATUS_AVAILABLE	0x04
#define C6_STATUS_VISIBLE_IP	0x01

#define C6_STATUS_MASK							\
	(C6_STATUS_BUSY | C6_STATUS_NETFRIENDS | C6_STATUS_AVAILABLE)   

#define C6_IS_ONLINE(_status)						\
	((_status) & C6_STATUS_MASK && (_status) != 0xff)

#endif
