
/* $Id: irc.h,v 1.1 2005/04/16 09:23:25 benkj Exp $ */

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

#ifndef __irc_defs__
#define __irc_defs__

SIMPLEQ_HEAD(command_list, command_entry);

struct command_entry {
	char *str;

	SIMPLEQ_ENTRY(command_entry) list;
};

typedef struct {
	int fd;
	FILE *fout;

	uint8_t conn_flags;
	uint8_t is_on_searchusr;
	
	unsigned int skip_colors:1;
	
	char buf[65536];
	uint16_t old_len;

	struct command_list cmd_list;
	
	struct in_addr ip;

	char *away;

	struct event ev;
} irc_t;


#include "irc/func.h"

#define IRC(_c)	((irc_t *)(_c)->gui_mod)

#define IRC_FLAG_PASS		0x1
#define IRC_FLAG_NICK		0x2
#define IRC_FLAG_USER		0x4
#define IRC_FLAG_READY		0x8
#define _IRC_LOGIN_COMPLETED						\
	(IRC_FLAG_PASS | IRC_FLAG_NICK | IRC_FLAG_USER)
#define _IRC_CONN_COMPLETED						\
	(IRC_FLAG_PASS | IRC_FLAG_NICK | IRC_FLAG_USER | IRC_FLAG_READY)

#define IRC_IS_LOGIN_COMPLETED(_c)					\
	((IRC(_c)->conn_flags & _IRC_LOGIN_COMPLETED) == _IRC_LOGIN_COMPLETED)
#define IRC_IS_CONNECTION_COMPLETED(_c)					\
	((IRC(_c)->conn_flags & _IRC_CONN_COMPLETED) == _IRC_CONN_COMPLETED)

#if 0
#define IRC_CHANMODES	"lvhopsmntikrRcaqOALQbSeKVfGCuzN"
#else
#define IRC_CHANMODES	"ovOkstlib"
#endif
#define IRC_USERMODES	"aniwroOs"

#define IRC_ISCHANNEL(_ch) ((_ch)[0] == '#' || (_ch)[0] == '&')
#define IRC_ISLOCHAN(_ch) ((_ch)[0] == '&')
#define IRC_ISREMCHAN(_ch) ((_ch)[0] == '#')

#define SERV		"irC6"
#define CONSOLE		"&irC6"
#define SEARCHUSR	"&RicercaUtenti"

#endif
