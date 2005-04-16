
/* $Id: console.c,v 1.1 2005/04/16 09:23:24 benkj Exp $ */

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
#include "socket.h"
#include "list.h"
#include "str_missing.h"

static void
c_add(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	int i;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare almeno un nick");
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

static void
c_del(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	int i;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare almeno un nick");
		return;
	}

	EMIT(c, "send users del", argc, argv);

	for (i = 0; i < argc; i++) {
		char *nick = argv[i];

		if (nicklist_del(c, c->nicks, nick))
			IRC_SEND_PART(c, nick, nick, SERV, CONSOLE);
		else
			IRC_ERRMSG(c, "nick %s non presente nella lista", nick);
	}
}

void
irc_join_console(c)
    connection_t *c;
{
	struct nicklist *nl;
	hash_t *tmp;

	IRC_SEND_JOIN(c, c->nick, c->nick, SERV, CONSOLE);
	IRC_SEND_TOPIC(c, SERV, c->nick, CONSOLE, ".: irC6 console :.");

	/* send nicknames */
	IRC_SEND_NAMREPLY(c, SERV, c->nick, "@", CONSOLE, "@", c->nick);

	NICKLIST_FOREACH(c->nicks, tmp, &nl) {
		IRC_SEND_NAMREPLY(c, SERV, c->nick, "@", CONSOLE,
				  (nl->status & STATUS_ONLINE) ? "%" : "+",
				  nl->name);
	}

	IRC_SEND_ENDNAMREPLY(c, SERV, c->nick, CONSOLE);

	IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV, CONSOLE, "+o", c->nick);

	IRC_CONSOLE(c,
		    "Questa e' la console di irC6, tramite questa finestra e' "
		    "possibile gestire tutte le operazioni. Digitare \"aiuto\" per "
		    "visualizzare i comandi supportati.");
}

static void
c_ignore(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	struct nicklist *nl;
	int i;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare almeno un nick");
		return;
	}

	EMIT(c, "send users req", argc, argv);

	for (i = 0; i < argc; i++) {
		char *nick = argv[i];

		switch (nicklist_add(c, c->nicks, nick)) {
		case 1:
			IRC_SEND_JOIN(c, nick, nick, SERV, CONSOLE);
			break;
		case 0:
			break;
		default:
			IRC_ERRMSG(c, "impossibile aggiungere %s alla lista",
				   nick);
		}

		if (!nicklist_getstruct(c->nicks, nick, &nl))
			FATAL("nicklist error");

		if (!nl->black_list) {
			if (nl->status & STATUS_ONLINE && 
			    (nl->status & STATUS_AVAILABLE ||
			     (nl->status & STATUS_MASK) == 0))
				IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
						  CONSOLE, "-o", nick);
			if (nl->status & STATUS_ONLINE && 
			    nl->status & (STATUS_BUSY | STATUS_NETFRIENDS))
				IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
						  CONSOLE, "-h", nick);

			IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
					  CONSOLE, "-v", nick);
		}

		nl->black_list = 1;
	}
}

static void
c_unignore(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	struct nicklist *nl;
	int i;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare almeno un nick");
		return;
	}

	for (i = 0; i < argc; i++) {
		if (!nicklist_getstruct(c->nicks, argv[i], &nl))
			IRC_CONSOLE(c, "nick non presente nella lista");

		if (nl->black_list) {
			IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
					  CONSOLE, "+v", argv[i]);

			if (nl->status & STATUS_ONLINE && 
			    (nl->status & STATUS_AVAILABLE || 
			     (nl->status & STATUS_MASK) == 0))
				IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
						  CONSOLE, "+o", argv[i]);
			if (nl->status & STATUS_ONLINE && 
			    nl->status & (STATUS_BUSY | STATUS_NETFRIENDS))
				IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
						  CONSOLE, "+h", argv[i]);
		}

		nl->black_list = 0;
	}
}

static void
c_userinfo(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	int i;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare almeno un nick");
		return;
	}

	for (i = 0; i < argc; i++) {
		if (IRC_ISREMCHAN(argv[i])) {
			CONNECT(c, "gui room info", irc_out_roominfo);

			EMIT(c, "send roominfo req", 
			     irc_room_decode(c->atmp, argv[i]));
		} else {
			CONNECT(c, "gui user info", irc_out_user_info);

			EMIT(c, "send userinfo req", argv[i]);
		}
	}
}

static void
c_style(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	if (argc < 1) {
		IRC_CONSOLE(c, "specificare un argomento; vedere `stile ?'");
		return;
	}

	if (!strcmp(argv[0], "?")) {
		char **desc;
		unsigned int elms, i;

		EMIT(c, "get styles", &elms, &desc);

		for (i = 0; i < elms; i++)
			IRC_CONSOLE(c, "stile %u -> %s", i, desc[i]);
	} else {
		unsigned int style = (unsigned int)strtoul(argv[0], NULL, 10);

		EMIT(c, "set style", &style);

		if (style == 0)
			IRC_CONSOLE(c, "argomento non valido; "
				    "usare `stile ?' per la lista di "
				    "argomenti accettati");
	}
}

static void
c_roomenc(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *ptr;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare una stanza");
		return;
	}

	ptr = strstr(str, argv[0]);
	if (ptr == NULL)
		EFATAL("error parsing arguments");


	IRC_CONSOLE(c, "%s ---> %s", ptr, irc_room_encode(c->atmp, ptr));
}

static void
c_roomdec(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *ptr;

	if (argc < 1) {
		IRC_CONSOLE(c, "specificare una stanza");
		return;
	}

	ptr = strstr(str, argv[0]);
	if (ptr == NULL)
		EFATAL("error parsing arguments");


	IRC_CONSOLE(c, "%s ---> %s", ptr, irc_room_decode(c->atmp, ptr));
}

static void
c_newroom(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *topic, *droom, pub = -1;

	if (argc < 2)
		goto usage;

	if (argv[1][0] == 'p') {
		if (argv[1][1] == 'u')
			pub = 1;
		else if (argv[1][1] == 'r')
			pub = 0;
	}

	if (pub == -1)
		goto usage;

	droom = irc_room_decode(c->atmp, argv[0]);

	topic = (argc > 2) ? strstr(str, argv[2]) : droom;
	if (topic == NULL)
		EFATAL("error parsing arguments");

	EMIT(c, "send room create", droom, pub, topic);

	return;
      usage:
	IRC_CONSOLE(c,
		    "usare: crea_stanza <stanza> <pubblica|privata> <topic>");
}

static void
c_mail(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *msg;

	if (argc < 2) {
		IRC_CONSOLE(c, "usare: mail <nick> <messaggio>");
		return;
	}

	msg = strstr(str, argv[1]);
	if (msg == NULL)
		EFATAL("error parsing arguments");

	EMIT(c, "send offline message", argv[0], (*msg == ':') ? msg + 1 : msg);
}

static void
c_status(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	if (argc > 0)
		switch (**argv) {
		case 'd':
			irc_status_available(c);
			return;
		case 'o':
			irc_status_busy(c, 1, NULL);
			return;
		case 'a':
			irc_status_onlyfriends(c, 1);
			return;
		}

	IRC_CONSOLE(c, "usare: stato <disponibile|occupato|amici>");
}

static void
c_searchnicks(c, argc, argv, str)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
    char *str __attribute__ ((unused));
{
	if (IRC(c)->is_on_searchusr)
		return;
	IRC(c)->is_on_searchusr = 1;

	EMIT(c, "reset search value");

	IRC_SEND_JOIN(c, c->nick, c->nick, SERV, SEARCHUSR);
	IRC_SEND_TOPIC(c, SERV, c->nick, SEARCHUSR, "Ricerca Utenti");

	IRC_SEND_NAMREPLY(c, SERV, c->nick, "@", SEARCHUSR, "@", c->nick);
	IRC_SEND_ENDNAMREPLY(c, SERV, c->nick, SEARCHUSR);

	IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV, SEARCHUSR, "+o", c->nick);

	IRC_SEARCHMSG(c, "Digitare \"aiuto\" per visualizzare i comandi "
		      "supportati.");
}
