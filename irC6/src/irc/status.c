
/* $Id: status.c,v 1.1 2005/04/16 09:23:26 benkj Exp $ */

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

#include "irC6.h"
#include "irc/irc.h"
#include "irc/msg.h"
#include "str_missing.h"
#include "error.h"

void
irc_status_busy(connection_t *c, int on, char *msg)
{
	c->status &= ~STATUS_MASK;

	if (on) {
		c->status |= STATUS_BUSY;

		if (msg != NULL) {
			IRC(c)->away = a_strdup(c->area, msg);

			PDEBUG("vado in away: %s", IRC(c)->away);
		}
	} else {
		c->status |= STATUS_AVAILABLE;

		if (IRC(c)->away != NULL) {
			a_free(c->area, IRC(c)->away);
			IRC(c)->away = NULL;
		}
	}

	EMIT(c, "send status");

	IRC_SEND_SRVMODE(c, c->nick, c->nick, (on) ? "+a" : "-a");
}

void
irc_status_onlyfriends(connection_t *c, int on)
{
	c->status &= ~STATUS_MASK;

	if (on)
		c->status |= STATUS_NETFRIENDS;
	else
		c->status |= STATUS_AVAILABLE;

	EMIT(c, "send status");

	IRC_SEND_SRVMODE(c, c->nick, c->nick, (on) ? "+n" : "-n");

	if (IRC(c)->away != NULL) {
		a_free(c->area, IRC(c)->away);
		IRC(c)->away = NULL;
	}
}

void
irc_status_available(connection_t *c)
{
	if (c->status & STATUS_BUSY)
		irc_status_busy(c, 0, NULL);
	if (c->status & STATUS_NETFRIENDS)
		irc_status_onlyfriends(c, 0);
}

void
irc_status_visibleip(connection_t *c, int on)
{
	if (on)
		c->status &= ~STATUS_VISIBLE_IP;
	else
		c->status |= STATUS_VISIBLE_IP;

	EMIT(c, "send status");

	IRC_SEND_SRVMODE(c, c->nick, c->nick, (on) ? "+i" : "-i");
}
