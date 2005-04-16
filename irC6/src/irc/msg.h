
/* $Id: msg.h,v 1.1 2005/04/16 09:23:25 benkj Exp $ */

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

#define IRC_RAWMSG(_c, _fmt, _args...)					\
	fprintf(IRC(_c)->fout, _fmt "\r\n" , ## _args)			

#include "irc/reply.h"

#define IRC_NOTICE(_c, _fmt, _arg...)					\
	IRC_RAWMSG(_c, ":%s NOTICE %s :" _fmt, SERV, _c->nick , ##_arg)

#define IRC_CONSOLE(_c, _fmt, _arg...)					\
	IRC_RAWMSG(_c, ":%s NOTICE %s :" _fmt, SERV, CONSOLE , ##_arg)

#define IRC_SEARCHMSG(_c, _fmt, _arg...)				\
	IRC_RAWMSG(_c, ":%s NOTICE %s :" _fmt, SERV, SEARCHUSR , ##_arg)

/* messaggi di errore */
#define IRC_WRNMSG(_c, _fmt, _args...)					\
	IRC_NOTICE(_c, "**ATTENZIONE** " _fmt , ## _args)

#define IRC_ERRMSG(_c, _fmt, _args...)					\
	IRC_NOTICE(_c, "**ERRORE** " _fmt , ## _args)

#define IRC_FATAL(_c, _fmt, _args...)					\
do {									\
	char _buf[1024];						\
	snprintf(_buf, sizeof(_buf), _fmt , ##_args);			\
	IRC_NOTICE((_c), "**ERRORE** %s", _buf);			\
	IRC_SEND_CQUIT(_c, (_c)->nick, inet_ntoa(IRC(_c)->ip), _buf);	\
	irC6_close_connection(_c);					\
	return;								\
} while (0)

