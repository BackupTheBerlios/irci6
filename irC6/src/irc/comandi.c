
/* $Id: comandi.c,v 1.1 2005/04/16 09:23:24 benkj Exp $ */

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
do_login(c, flag)
    connection_t *c;
    uint8_t flag;
{
	if (IRC_IS_LOGIN_COMPLETED(c))
		return;

	IRC(c)->conn_flags |= flag;

	if (IRC_IS_LOGIN_COMPLETED(c)) {
		PDEBUG("LOGIN nick(%s) pass(%s) user(%s)",
		       c->nick, c->pass, c->user);

		/* remove timeout */
		event_del(&IRC(c)->ev);
		event_add(&IRC(c)->ev, NULL);

		EMIT(c, "connect", server_host, server_port);
	}
}

#define ADD_COMMAND(_c, _str)						\
do {									\
	if (!IRC_IS_CONNECTION_COMPLETED(c)) {				\
		struct command_entry *cmd;				\
	    								\
	    	PDEBUG("salvo comando: %s", _str);			\
	    								\
		cmd = a_new(_c->area, struct command_entry);		\
	    								\
		cmd->str = a_strdup(_c->area, _str);			\
		SIMPLEQ_INSERT_TAIL(&IRC(_c)->cmd_list, cmd, list);	\
	    								\
		return;							\
	}								\
} while (0)

static void
c_nick(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	if (IRC_IS_LOGIN_COMPLETED(c)) {
		IRC_ERRMSG(c, "login gia' completata");
		return;
	}

	if (argc > 0) {
		PDEBUG("ricevuto NICK %s", argv[0]);

		strlcpy(c->nick, argv[0], sizeof(c->nick));

		do_login(c, IRC_FLAG_NICK);
	} else
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "NICK");
}

static void
c_pass(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	if (IRC_IS_LOGIN_COMPLETED(c)) {
		IRC_ERRMSG(c, "login gia' completata");
		return;
	}

	if (argc > 0) {
		PDEBUG("ricevuto PASS %s", argv[0]);

		strlcpy(c->pass, argv[0], sizeof(c->pass));

		do_login(c, IRC_FLAG_PASS);
	} else
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "PASS");
}

static void
c_user(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	int i;

	if (IRC_IS_LOGIN_COMPLETED(c)) {
		IRC_ERRMSG(c, "login gia' completata");
		return;
	}

	if (argc < 3) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "USER");
		return;
	}

	strlcpy(c->user, argv[0], sizeof(c->user));

	PDEBUG("ricevuto user(%s", c->user);

	do_login(c, IRC_FLAG_USER);
}

static void
c_ping(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str __attribute__ ((unused));
{
	if (argc < 1) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "PING");
		return;
	}
	PDEBUG("ricevuto PING (%s)", argv[0]);

	IRC_SEND_PONG(c, SERV, SERV, (*argv[0] == ':') ? argv[0] + 1 : argv[0]);
	PDEBUG("invio :irC6 PONG %s", argv[0]);
}

static void
nick_mode(c, svmode, arg, str)
    connection_t *c;
    char *svmode, *arg, *str;
{
	char *ptr, *away, mode[8];
	int plus = 1;

	for (ptr = svmode; *ptr != '\0'; ptr++)
		switch (*ptr) {
		case '+':
			plus = 1;
			break;
		case '-':
			plus = 0;
			break;
		case 'a':
			if (plus && arg != NULL) {
				away = strstr(str, (*arg == ':') ?
					      arg + 1 : arg);

				if (away == NULL)
					EFATAL("error parsing arguments");
			} else
				away = NULL;

			irc_status_busy(c, plus, away);

			break;
		case 'n':
			irc_status_onlyfriends(c, plus);

			break;
		case 'i':
			irc_status_visibleip(c, plus);

			break;
		case 'k':
			IRC(c)->skip_colors = plus ? 1 : 0;
			break;
		default:
			IRC_SEND_UNKMODE(c, SERV, c->nick, *ptr);
		}
}

static void
chan_mode(c, chan, mode, arg)
    connection_t *c;
    char *chan, *mode, *arg;
{
	char *ptr;
	int plus = 1;

	for (ptr = mode; *ptr != '\0'; ptr++)
		switch (*ptr) {
		case '+':
			plus = 1;
			break;
		case '-':
			plus = 0;
			break;
		case 'b':
			if (arg == NULL)
				IRC_SEND_ENDBANLIST(c, SERV, c->nick, chan);
			else
				IRC_SEND_CANTCHANGE(c, SERV, c->nick, chan);
			break;
		case 'e':
			if (arg == NULL)
				IRC_SEND_ENDEXCEPLIST(c, SERV, c->nick, chan);
			else
				IRC_SEND_CANTCHANGE(c, SERV, c->nick, chan);
			break;
		case 'I':
			if (arg == NULL)
				IRC_SEND_ENDINVLIST(c, SERV, c->nick, chan);
			else
				IRC_SEND_CANTCHANGE(c, SERV, c->nick, chan);
			break;
		case 'o':
		case 'v':
		case 'O':
		case 'k':
		case 's':
		case 't':
		case 'l':
		case 'i':
			IRC_SEND_CANTCHANGE(c, SERV, c->nick, chan);
			break;
		default:
			IRC_SEND_UNKMODE(c, SERV, chan, *ptr);
		}
}

static void
c_mode(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	extern time_t srv_time;

	ADD_COMMAND(c, str);

	if (argc < 2) {
		if (IRC_ISCHANNEL(argv[0])) {
			IRC_SEND_CHANMODEIS(c, SERV, c->nick, argv[0], "+", "");
			IRC_SEND_CREATIONTIME(c, SERV, c->nick, argv[0],
					      (unsigned long)srv_time);
		} else
			IRC_SEND_ERRPARAM(c, SERV, c->nick, "MODE");
		return;
	}
	PDEBUG("ricevuto MODE (%s) (%s)", argv[0], argv[1]);

	if (!strcmp(c->nick, argv[0]))
		nick_mode(c, argv[1], (argc > 2) ? argv[2] : NULL, str);
	else if (IRC_ISCHANNEL(argv[0]))
		chan_mode(c, argv[0], argv[1], (argc > 2) ? argv[2] : NULL);

	PDEBUG("invio :irC6 MODE %s", argv[0]);
}

static void
c_quit(c, argc, argv, str)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
    char *str __attribute__ ((unused));
{
	IRC_SEND_CQUIT(c, c->nick, inet_ntoa(IRC(c)->ip), "Quit: Client exit");
	irC6_close_connection(c);
}

static void
c_squit(c, argc, argv, str)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
    char *str __attribute__ ((unused));
{
	if (can_squit)
		exit(EXIT_SUCCESS);
	else
		IRC_SEND_UNKCMD(c, SERV, c->nick, "SQUIT");
}

static void
c_privmsg(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *msg, *arg0;

	ADD_COMMAND(c, str);

	if (argc < 2) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "PRIVMSG");
		return;
	}

	arg0 = (*argv[0] == ':') ? argv[0] + 1 : argv[0];


	if (IRC_ISLOCHAN(arg0)) {
		if (!strcmp(arg0, CONSOLE))
			irc_console_input(c, argc - 1, argv + 1, str);
  		if (!strcmp(arg0, SEARCHUSR))
    			irc_searchusr_input(c, argc -1 , argv + 1);
	} else {
		msg = strstr(str, argv[1]);
		if (msg == NULL)
			EFATAL("error parsing arguments");

		if (*msg == ':')
			msg++;

		if (IRC_ISREMCHAN(arg0))
			EMIT(c, "send room message", 
			     irc_room_decode(c->atmp, arg0), msg);
		else
			EMIT(c, "send message", argv[0], msg);
	}
}

static void
c_ison(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	int i;

	ADD_COMMAND(c, str);

	if (argc > 0) {
		if (argv[0][0] == ':')
			argv[0] = argv[0] + 1;

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
				IRC_ERRMSG(c,
					   "impossibile aggiungere %s alla lista",
					   nick);
			}
		}
	} else
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "ISON");
}

static void
c_part(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *chan, *p;

	ADD_COMMAND(c, str);

	if (argc < 1) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "PART");
		return;
	}

	chan = a_strdup(c->atmp, (argv[0][0] == ':') ?
			argv[0] + 1 : argv[0]);

	while ((p = strsep(&chan, ",")) != NULL) {
		if (IRC_ISLOCHAN(p)) {
			if (!strcmp(p, CONSOLE))
				irc_join_console(c);
			else if (!strcmp(p, SEARCHUSR)) {
				if (!IRC(c)->is_on_searchusr)
					IRC_SEND_NOTONCHANNEL(c, SERV,
							      c->nick,
							      SEARCHUSR);
				else {
					IRC_SEND_PART(c, c->nick, c->user,
						      inet_ntoa(IRC(c)->ip),
						      SEARCHUSR);

					IRC(c)->is_on_searchusr = 0;
				}
			}
		} else if (IRC_ISREMCHAN(p))
			EMIT(c, "send room leave", irc_room_decode(c->atmp, p));
	}
}

static void
c_join(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *chan, *p;

	ADD_COMMAND(c, str);

	if (argc < 1) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "JOIN");
		return;
	}

	chan = a_strdup(c->atmp, (argv[0][0] == ':') ?
			argv[0] + 1 : argv[0]);

	while ((p = strsep(&chan, ",")) != NULL)
		if (IRC_ISREMCHAN(p))
			EMIT(c, "send room enter", irc_room_decode(c->atmp, p));
}

static void
who_console(c)
    connection_t *c;
{
	hash_t *tmp;
	struct nicklist *nl;

	IRC_SEND_WHOREPLY(c, SERV, c->nick, CONSOLE, c->nick, SERV, SERV,
			  c->nick, "H@", 0, "local operator");

	NICKLIST_FOREACH(c->nicks, tmp, &nl) {
		IRC_SEND_WHOREPLY(c, SERV, c->nick, CONSOLE,
				  nl->name, SERV, SERV, nl->name,
				  (nl->status & STATUS_ONLINE) ? "H%" : "H+",
				  0, "netfriend");
	}
}

static void
who_searchusr(c)
    connection_t *c;
{
	if (!IRC(c)->is_on_searchusr)
		return;

	IRC_SEND_WHOREPLY(c, SERV, c->nick, SEARCHUSR, c->nick, SERV, SERV,
			  c->nick, "H@", 0, "local operator");
}

static void
who_channel(c, chan)
    connection_t *c;
    char *chan;
{
	char *nick;
	struct chanlist *cl;
	hash_t *tmp;

	if (!chanlist_getstruct(c->chans, irc_room_decode(c->atmp, chan), &cl))
		return;

	IRC_SEND_WHOREPLY(c, SERV, c->nick, chan, c->nick, SERV, SERV, c->nick,
			  "H", 0, "local nick");

	CHANLIST_FOREACH_NICK(cl, tmp, &nick) {
		IRC_SEND_WHOREPLY(c, SERV, c->nick, chan, nick, SERV, SERV,
				  nick, "H", 0, "netfriend");
	}
}

static void
who_allchan(c)
    connection_t *c;
{
	char *nick;
	struct chanlist *cl;
	hash_t *tmp1, *tmp2;

	CHANLIST_FOREACH(c->chans, tmp1, &cl) {
		IRC_SEND_WHOREPLY(c, SERV, c->nick, cl->cname, c->nick,
				  SERV, SERV, c->nick, "H", 0, "local nick");

		CHANLIST_FOREACH_NICK(cl, tmp2, &nick) {
			IRC_SEND_WHOREPLY(c, SERV, c->nick, cl->cname, nick,
					  SERV, SERV, nick, "H", 0,
					  "netfriend");
		}
	}
}

static void
c_who(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *arg, *mask;

	ADD_COMMAND(c, str);

	if (argc == 0 || !strcmp(argv[0], "0")) {
		who_console(c);
		who_searchusr(c);
		who_allchan(c);

		IRC_SEND_ENDWHOREPLY(c, SERV, c->nick, "*");
		return;
	}

	mask = a_strdup(c->atmp, argv[0]);

	while ((arg = strsep(&mask, ",")) != NULL) {
		if (IRC_ISCHANNEL(arg)) {
			if (!strcmp(arg, CONSOLE))
				who_console(c);
			else if (!strcmp(arg, SEARCHUSR))
				who_searchusr(c);
			else
				who_channel(c, arg);
		} else
			IRC_SEND_WHOREPLY(c, SERV, c->nick, "*", arg, SERV,
					  SERV, arg, "H", 0, "netfriend");

		IRC_SEND_ENDWHOREPLY(c, SERV, c->nick, arg);
	}
}

static void
c_away(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *arg[3];

	ADD_COMMAND(c, str);

	arg[0] = c->nick;
	if (argc > 0) {
		arg[1] = "+a";
		arg[2] = argv[0];
		c_mode(c, 3, arg, str);
		IRC_SEND_NOWAWAY(c, SERV, c->nick);
	} else {
		arg[1] = "-a";
		c_mode(c, 2, arg, str);
		IRC_SEND_UNAWAY(c, SERV, c->nick);
	}
}

static void
c_whois(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	int i;

	ADD_COMMAND(c, str);

	if (argc < 1) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "WHOIS");
		return;
	}

	for (i = 0; i < argc; i++) {
		PDEBUG("whois %s", argv[i]);
		
		CONNECT(c, "gui user info", irc_out_whois);

		EMIT(c, "send userinfo req", argv[i]);
	}
}

static void
c_mail(c, argc, argv, str)
    connection_t *c;
    int argc;
    char **argv;
    char *str;
{
	char *msg;

	ADD_COMMAND(c, str);

	if (argc < 2) {
		IRC_SEND_ERRPARAM(c, SERV, c->nick, "MAIL");
		return;
	}

	msg = strstr(str, argv[1]);
	if (msg == NULL)
		EFATAL("error parsing arguments");

	EMIT(c, "send offline message", argv[0], (*msg == ':') ? msg + 1 : msg);
}

static void
c_list(c, argc, argv, str)
    connection_t *c;
    int argc __attribute__ ((unused));
    char **argv __attribute__ ((unused));
    char *str __attribute__ ((unused));
{
	EMIT(c, "send roomlist req");
}
