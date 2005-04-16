
/* $Id: searchusr.c,v 1.1 2005/04/16 09:23:26 benkj Exp $ */

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
#include "error.h"
#include "list.h"
#include "str_missing.h"

static void
c_checkword(c, argc, argv)
    connection_t *c;
    int argc;
    char **argv;
{
	char *arg0, *arg2, *argv0;
	int id, val;

	if (argc < 3 || strcmp(argv[1], "=")) {
		IRC_SEARCHMSG(c, "usare: campo = valore");
		return;
	}

	argv0 = (*argv[0] == ':') ? argv[0] + 1 : argv[0];
	arg0 = argv0;
	arg2 = argv[2];

	EMIT(c, "set search value", &arg0, &arg2);

	if (arg0 == NULL)
		IRC_SEARCHMSG(c, "%s: campo sconosciuto", argv0);
	else if (arg2 == NULL)
		IRC_SEARCHMSG(c, "%s: valore non valido per %s",
			      argv[2], argv0);
}

static void
c_showconf(c, argc, argv)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
{
	unsigned int i, elms;
	char **idv, **valv;

	EMIT(c, "get set search values", &elms, &idv, &valv);

	for (i = 0; i < elms; i++)
		IRC_SEARCHMSG(c, "  %s = %s", idv[i], valv[i]);
}

static void
c_reset(c, argc, argv)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
{
	EMIT(c, "reset search value");
}

static void
c_search(c, argc, argv)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
{
	EMIT(c, "send search users");
}

static void
c_list(c, argc, argv)
    connection_t *c;
    int argc;
    char **argv;
{
	char **clist = argv;
	unsigned int celms = (unsigned int)argc;
	char *name, **elist;
	unsigned int i, k, eelms;

	if (argc == 0)
		EMIT(c, "get search values", NULL, &celms, &clist);

	for (i = 0; i < celms; i++) {
		if (argc > 0) {
			EMIT(c, "get search values", clist[i], &eelms, &elist);

			if (eelms == 0) {
				IRC_SEARCHMSG(c, "campo non valido: `%s'",
					      clist[i]);

				continue;
			}

			for (k = 0; k < eelms; k++)
				IRC_SEARCHMSG(c, "%s:     [%i] %s", clist[i],
					      k, elist[k]);
		} else
			IRC_SEARCHMSG(c, "  [%i] %s", i, clist[i]);
	}
}

static void
c_email(c, argc, argv)
    connection_t *c;
    int argc;
    char **argv;
{
	int i;

	if (argc <= 0) {
		IRC_SEARCHMSG(c, "specificare almeno un indirizzo email");
		return;
	}

	for (i = 0; i < argc; i++)
		EMIT(c, "send search by email", argv[i]);
}

static void
c_add(c, argc, argv)
    connection_t *c;
    int argc;
    char **argv;
{
	int i;

	if (argc < 1) {
		IRC_SEARCHMSG(c, "specificare almeno un nick");
		return;
	}

	EMIT(c, "send users req", argc, argv);

	for (i = 0; i < argc; i++) {
		char *nick = argv[i];

		switch (nicklist_add(c, c->nicks, nick)) {
		case 1:
			IRC_SEND_JOIN(c, nick, nick, SERV, CONSOLE);

			IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
					  CONSOLE, "+v", nick);
			break;
		case 0:
			IRC_ERRMSG(c, "nick %s gia' presente", nick);
			break;
		default:
			IRC_ERRMSG(c, "impossibile aggiungere %s alla lista",
				   nick);
		}
	}
}
