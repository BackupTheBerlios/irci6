
/* $Id: irC6.h,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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

#ifndef __irC6_h
#define __irC6_h

#include "defs.h"
#include "area.h"
#include "buffer.h"
#include "hash.h"
#include "signals.h"
#include <time.h>

typedef struct connection_entry {
	void *proto_mod;
	void *gui_mod;

	char nick[32];
	char user[32];
	char pass[32];

	hashtable_t *nicks;
	hashtable_t *chans;

	jmp_buf jmp;
	siginst_t *siginst;

	area_t *area;
	area_t *atmp;

	uint8_t status;

	SLIST_ENTRY(connection_entry) list; 
} connection_t;

#define STATUS_ONLINE		0x01
#define STATUS_BUSY		0x02
#define STATUS_NETFRIENDS	0x04
#define STATUS_AVAILABLE	0x08
#define STATUS_VISIBLE_IP	0x10

#define STATUS_MASK							\
	(STATUS_BUSY | STATUS_NETFRIENDS | STATUS_AVAILABLE)   

struct module {
	void (*init)();
	void (*deinit)();
	void (*connection_new)(connection_t *);
	void (*connection_free)(connection_t *);
};

extern siginst_t *ev_root;

extern char *listen_host, *listen_port;
extern char *server_host, *server_port;

extern int is_daemon, can_squit, use_inetd, max_conn;

connection_t *irC6_new_connection();
void irC6_close_connection(connection_t *);

#define EMIT(_c, _name, _args...)					\
	sig_emit((_c)->siginst, _name, _c , ##_args)
#define EMIT_ID(_c, _id, _args...)					\
	sig_emit_id((_c)->siginst, _id, _c , ##_args)

#define EMIT_FMT(_c, _name, _fmt, _args...) do {			\
	char _tmp[1024];						\
	snprintf(_tmp, sizeof(_tmp), _fmt , ##_args);			\
	sig_emit((_c)->siginst, _name, _c, _tmp);			\
} while (0)
#define EMIT_ID_FMT(_c, _id, _fmt, _args...) do {			\
	char _tmp[1024];						\
	snprintf(_tmp, sizeof(_tmp), _fmt , ##_args);			\
	sig_emit_id((_c)->siginst, _id, _c, _tmp);			\
} while (0)

#define CONNECT(_c, _sig, _cb)						\
	sig_connect_opt((_c)->siginst, _sig, SIGNALS_FIRST, 1, _cb)

#define irC6_callback_start(c) do {					\
	if (setjmp(c->jmp)) {						\
		return;							\
	}								\
	c->atmp = area_node(c->area);					\
} while (0)

#define irC6_callback_end(c) do {					\
	area_destroy(c->atmp);						\
} while (0)

#ifdef IRC6_DEBUG
#define CDEBUG(x)	do { x } while (0)
#define PDEBUG(fmt, args...)						\
do {									\
	if (is_daemon)							\
		syslog(LOG_DEBUG, "%s[%d]: " fmt, 			\
		    __FILE__ , __LINE__ , ## args);			\
	else								\
		fprintf(stderr, "%s[%d]: " fmt "\n", 			\
		    __FILE__ , __LINE__ , ## args);			\
} while (0)
#else
#define CDEBUG(x)
#define PDEBUG(fmt, args...)
#endif

#endif
