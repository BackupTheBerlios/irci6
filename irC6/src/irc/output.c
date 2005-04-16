
/* $Id: output.c,v 1.1 2005/04/16 09:23:25 benkj Exp $ */

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
#include "version.h"
#include "str_missing.h"

void
irc_out_fatal(connection_t *c, char *err)
{
	IRC_FATAL(c, "%s", err);
}

void
irc_out_warning(connection_t *c, char *err)
{
	IRC_WRNMSG(c, "%s", err);
}

void
irc_out_quit(connection_t *c, char *msg)
{
	IRC_SEND_CQUIT(c, c->nick, inet_ntoa(IRC(c)->ip), msg);
	irC6_close_connection(c);
}

void
irc_out_welcome(connection_t *c, char *msg)
{
	char *ptr, motdhead[256], mbuf[80], str_time[256];
	size_t linemax;
	struct command_entry *cmd, *cmd_next;
	extern time_t srv_time;
	struct tm ltime;

	IRC_NOTICE(c, "-");

	IRC_SEND_WELCOME(c, SERV, c->nick, SERV, c->nick, c->user,
			 inet_ntoa(IRC(c)->ip));
	IRC_SEND_YOURHOST(c, SERV, c->nick, SERV, VERSION);

	localtime_r(&srv_time, &ltime);
	strftime(str_time, sizeof(str_time), "%a %b %e %Y at %H:%M:%S %Z",
		 &ltime);
	IRC_SEND_CREATED(c, SERV, c->nick, str_time);

	IRC_SEND_MYINFO(c, SERV, c->nick, SERV, VERSION, IRC_USERMODES,
			IRC_CHANMODES);

	IRC_SEND_MOTDSTART(c, SERV, c->nick, SERV);

	/* FIXME: */
	snprintf(motdhead, 256, ":%s 372 %s :- ", SERV, c->nick);
	linemax = 80 - strlen(motdhead);

	for (ptr = msg; *ptr != '\0'; ptr++) {
		if (*ptr == '\n') {
			if ((ptr - msg) > 0 && ptr[-1] == '\r')
				ptr[-1] = '\0';
			else
				*ptr = '\0';

			IRC_SEND_MOTD(c, SERV, c->nick, msg);

			msg = ptr;
		}
		if (linemax == (size_t)(ptr - msg)) {
			memcpy(mbuf, msg, ptr - msg);
			mbuf[ptr - msg] = '\0';

			IRC_SEND_MOTD(c, SERV, c->nick, mbuf);

			msg = ptr;
		}
	}

	if (msg != ptr) {
		memcpy(mbuf, msg, ptr - msg);
		mbuf[ptr - msg] = '\0';

		IRC_SEND_MOTD(c, SERV, c->nick, msg);
	}

	IRC_SEND_MOTDEND(c, SERV, c->nick);

	IRC_SEND_SRVMODE(c, c->nick, c->nick, "+O");

	irc_join_console(c);

	IRC(c)->conn_flags |= IRC_FLAG_READY;

	for (cmd = SIMPLEQ_FIRST(&IRC(c)->cmd_list);
	     cmd != SIMPLEQ_END(&IRC(c)->cmd_list); cmd = cmd_next) {
		cmd_next = SIMPLEQ_NEXT(cmd, list);

		irc_process_input(c, cmd->str, strlen(cmd->str));

		a_free(c->area, cmd->str);
		a_free(c->area, cmd);
	}

	SIMPLEQ_INIT(&IRC(c)->cmd_list);
}

void
irc_out_invuser(connection_t *c)
{
	IRC_SEND_NICKERR(c, SERV, c->nick, c->nick, "utente inesistente");
	IRC_FATAL(c, "utente %s inesistente", c->nick);
}

void
irc_out_invpass(connection_t *c)
{
	IRC_SEND_PASSERR(c, SERV, c->nick);
	IRC_FATAL(c, "password non valida");
}

void
irc_out_alrconn(connection_t *c)
{
	IRC_SEND_NICKINUSE(c, SERV, c->nick, c->nick);
	IRC_FATAL(c, "utente %s gia' collegato", c->nick);
}

void
irc_out_logindone(connection_t *c, struct sockaddr_in *addr)
{
	IRC_NOTICE(c, "Benvenuti in irC6 v%s", VERSION);
	IRC_NOTICE(c, "Copyright: Leonardo Banchi <benkj@antifork.org>");
	IRC_NOTICE(c, "Siete connessi al server C6 %s:%hu",
		   inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
	IRC_NOTICE(c, "-");
	IRC_NOTICE(c, "-");
	IRC_NOTICE(c, "-");

	c->status = STATUS_ONLINE | STATUS_AVAILABLE;
}

void
irc_out_banners(connection_t *c, unsigned int argc, char **argv)
{
	unsigned int i;

	for (i = 0; i < argc; i++)
		IRC_NOTICE(c, "%s", argv[i]);
}

void
irc_out_message(connection_t *c, char *from, char *to, char *msg, char *style)
{
	char *ptr;
	struct nicklist *nl;

	if (nicklist_getstruct(c->nicks, from, &nl) && nl->black_list) {
		PDEBUG("scarto messaggio di %s: %s", from, msg);
		return;
	}

	while ((ptr = strsep(&msg, "\n\r")) != NULL)
		if (*ptr != '\0')
			IRC_SEND_PRIVMSG(c, from, to, style, ptr);

	if (IRC(c)->away) {
		PDEBUG("invio away: %s", IRC(c)->away);

		EMIT(c, "send message", from, IRC(c)->away);
	}
}

void
irc_out_pkerror(connection_t *c, unsigned int pkt)
{
	IRC_ERRMSG(c, "messaggio %u non valido", pkt);
}

void
irc_out_users(connection_t *c, unsigned int argc, char **argv)
{
	unsigned int i;
	struct nicklist *nl;

	for (i = 0; i < argc; i++) {
		if (nicklist_getstruct(c->nicks, argv[i], &nl)) {
			if (!(nl->status & STATUS_ONLINE)) {
				if (!nl->black_list)
					IRC_SEND_CHANMODE(c, c->nick, c->nick,
							  SERV, CONSOLE, "+o",
							  argv[i]);

				nl->status |= STATUS_ONLINE;
			}
		} else
			IRC_ERRMSG(c, "nick %s non presente nella lista",
				   argv[i]);
	}
}


void
irc_out_user_online(connection_t *c, char *nick)
{
	struct nicklist *nl;

	if (!nicklist_getstruct(c->nicks, nick, &nl)) {
		IRC_ERRMSG(c, "nick %s non presente nella lista", nick);
		return;
	}

	nl->status |= STATUS_ONLINE;

	if (!nl->black_list)
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "+o", nick);

	IRC_CONSOLE(c, "%s e' online", nick);
}

void
irc_out_user_offline(connection_t *c, char *nick)
{
	struct nicklist *nl;

	if (!nicklist_getstruct(c->nicks, nick, &nl)) {
		IRC_ERRMSG(c, "nick %s non presente nella lista", nick);
		return;
	}


	if (!nl->black_list) {
		if (nl->status & STATUS_AVAILABLE ||
		    (nl->status & STATUS_MASK) == 0)
			IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
					  CONSOLE, "-o", nick);
		else
			IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
					  CONSOLE, "-h", nick);
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "+v", nick);
	}

	nl->status &= ~(STATUS_ONLINE | STATUS_MASK);
	IRC_CONSOLE(c, "%s e' offline", nick);
}

void
irc_out_user_chg_status(connection_t *c, char *nick, unsigned int status)
{
	struct nicklist *nl;

	if (!nicklist_getstruct(c->nicks, nick, &nl)) {
		IRC_ERRMSG(c, "nick %s non presente nella lista", nick);
		return;
	}

	IRC_NOTICE(c, "old status %x new %x", nl->status, status & 0xff);

	if (nl->status & (STATUS_BUSY | STATUS_NETFRIENDS) && 
	    status & STATUS_AVAILABLE) {
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "-h", nick);
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "+o", nick);
	}
	
	if (status & (STATUS_BUSY | STATUS_NETFRIENDS) && 
	    nl->status & STATUS_AVAILABLE) {
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "-o", nick);
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "+h", nick);
	}

	if ((nl->status & STATUS_MASK) == 0 && 
	    status & (STATUS_BUSY | STATUS_NETFRIENDS)) {
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "-o", nick);
		IRC_SEND_CHANMODE(c, c->nick, c->nick, SERV,
				  CONSOLE, "+h", nick);
	}

	nl->status = status;
}

void
irc_out_user_info(connection_t *c, char *nick, char *status,
		  struct tm *ltime, struct in_addr *ip,
		  unsigned int infoc, char **infov)
{
	char time_str[1024];
	unsigned int i;

	strftime(time_str, sizeof(time_str),
		 "Connesso dalle %T del %d/%m/%Y", ltime);

	if (ip) {
		IRC_CONSOLE(c, "%s: %s: %s", nick, time_str, status);
		IRC_CONSOLE(c, "%s: Indirizzo IP: %s", nick, inet_ntoa(*ip));
	} else
		IRC_CONSOLE(c, "%s: Non connesso", nick);

	for (i = 0; i < infoc; i++) {
		IRC_CONSOLE(c, "%s: %s: %s", nick,
			    infov[i * 2], infov[(i * 2) + 1]);
	}

	IRC_CONSOLE(c, "%s: Fine profilo", nick);
}

void
irc_out_whois(connection_t *c, char *nick, char *status,
	      struct tm *ltime, struct in_addr *ip,
	      unsigned int infoc, char **infov)
{
	char time_str[1024], bstr[1024];
	unsigned int i;

	strftime(time_str, sizeof(time_str),
		 "Connesso dalle %T del %d/%m/%Y", ltime);

	if (ip) {
		IRC_SEND_WHOISUSER(c, SERV, c->nick, nick,
				   nick, SERV, SERV " user");

		snprintf(bstr, sizeof(bstr), "    %s: %s", time_str, status);

		IRC_SEND_WHOISSPECIAL(c, SERV, c->nick, nick, bstr);

		snprintf(bstr, sizeof(bstr), "    Indirizzo IP: %s",
			 inet_ntoa(*ip));
		IRC_SEND_WHOISSPECIAL(c, SERV, c->nick, nick, bstr);

		for (i = 0; i < infoc; i++) {
			snprintf(bstr, sizeof(bstr), "    %s: %s",
				 infov[i * 2], infov[(i * 2) + 1]);

			IRC_SEND_WHOISSPECIAL(c, SERV, c->nick, nick, bstr);
		}

		IRC_SEND_ENDOFWHOIS(c, SERV, c->nick, nick);
	} else {
		IRC_SEND_WHOWASUSER(c, SERV, c->nick, nick,
				    nick, SERV, SERV " user");
		IRC_SEND_ENDOFWHOWAS(c, SERV, c->nick, nick);
	}
}

void
irc_out_roomlist(connection_t *c, unsigned int roomc, char **roomv,
		 unsigned int *usersv, unsigned int *pubv)
{
	unsigned int i;
	char *croom, desc[256];

	for (i = 0; i < roomc; i++) {
		croom = irc_room_encode(c->atmp, roomv[i]);

		snprintf(desc, sizeof(desc), "%s -- Canale %s", roomv[i],
			 pubv[i] ? "pubblico" : "privato");

		IRC_SEND_LIST(c, SERV, c->nick, croom, usersv[i], desc);
	}

	IRC_SEND_LISTEND(c, SERV, c->nick);
}

void
irc_out_roominfo(connection_t *c, char *room, char *topic, char *founder,
		 unsigned int users, unsigned int pub)
{
	char *croom;

	croom = irc_room_encode(c->atmp, room);

	IRC_CONSOLE(c, "%s: Nome: %s", croom, room);
	IRC_CONSOLE(c, "%s: %hu utente/i; Canale %s", croom, users,
		    pub ? "pubblico" : "privato");
	IRC_CONSOLE(c, "%s: Topic: %s", croom, topic);
	IRC_CONSOLE(c, "%s: Fondatore: %s", croom, founder);
}

void
irc_out_roomtopic(connection_t *c, char *room, char *topic, char *founder,
		  unsigned int users, unsigned int pub)
{
	char *croom, tmp[1024];

	croom = irc_room_encode(c->atmp, room);
	snprintf(tmp, sizeof(tmp), "%s -- %s", room, topic);

	IRC_SEND_TOPIC(c, SERV, c->nick, croom, tmp);
}

void
irc_out_room_ulist(connection_t *c, char *room, unsigned int usersc,
		   char **usersv)
{
	struct chanlist *cl;
	char *croom;
	unsigned int i;

	PDEBUG("joining room %s", room);
	
	croom = irc_room_encode(c->atmp, room);

	switch (chanlist_add(c, c->chans, room, croom)) {
	case 1:
		IRC_SEND_JOIN(c, c->nick, c->nick, SERV, croom);
		IRC_SEND_TOPIC(c, SERV, c->nick, croom, room);
		IRC_SEND_NAMREPLY(c, SERV, c->nick, "@", croom, "", c->nick);

		if (!chanlist_getstruct(c->chans, room, &cl))
			FATAL("chanlist error");

		for (i = 0; i < usersc; i++) {
			IRC_SEND_NAMREPLY(c, SERV, c->nick, "@", croom,
					  "", usersv[i]);

			if (chanlist_addnick(cl, usersv[i]) <= 0)
				IRC_ERRMSG(c,
					   "impossibile aggiungere %s alla lista",
					   usersv[i]);
		}

		/* remove own nick from list */
		chanlist_delnick(cl, c->nick);

		IRC_SEND_ENDNAMREPLY(c, SERV, c->nick, croom);

		CONNECT(c, "gui room info", irc_out_roomtopic);

		EMIT(c, "send roominfo req", room);
		break;
	case 0:
		IRC_ERRMSG(c, "siete gia' in %s ", croom);
		break;
	default:
		IRC_ERRMSG(c, "impossibile aggiungere %s alla lista", room);
	}
}

void
irc_out_room_message(connection_t *c, char *room, char *from, char *to,
		     char *msg, char *style)
{
	char *croom, *ptr;

	croom = irc_room_encode(c->atmp, room);

	while ((ptr = strsep(&msg, "\n\r")) != NULL)
		if (*ptr != '\0')
			IRC_SEND_PRIVMSG(c, from, croom, style, ptr);
}

void
irc_out_room_uenter(connection_t *c, char *room, char *user)
{
	char *croom;
	struct chanlist *cl;

	if (!chanlist_getstruct(c->chans, room, &cl)) {
		IRC_ERRMSG(c, "un utente e' entrato in una stanza inesistente");

		return;
	}

	if (chanlist_addnick(cl, user) <= 0)
		IRC_ERRMSG(c, "impossibile aggiungere %s alla lista", user);
	else {
		croom = irc_room_encode(c->atmp, room);

		IRC_SEND_JOIN(c, user, user, SERV, croom);
	}
}

void
irc_out_room_uleave(connection_t *c, char *room, char *user)
{
	char *croom;
	struct chanlist *cl;

	if (!chanlist_getstruct(c->chans, room, &cl)) {
		IRC_ERRMSG(c, "un utente e' uscito da una stanza inesistente");

		return;
	}

	if (!chanlist_delnick(cl, user))
		IRC_ERRMSG(c, "impossibile togliere %s dalla lista", user);
	else {
		croom = irc_room_encode(c->atmp, room);

		IRC_SEND_PART(c, user, user, SERV, croom);
	}
}

void
irc_out_room_canleave(connection_t *c, char *room)
{
	char *croom;

	croom = irc_room_encode(c->atmp, room);

	if (!chanlist_del(c, c->chans, room))
		IRC_SEND_NOTONCHANNEL(c, SERV, c->nick, croom);
	else
		IRC_SEND_PART(c, c->nick, c->nick, SERV, croom);
}

void
irc_out_room_notexist(connection_t *c, char *room)
{
	IRC_SEND_NOSUCHCHANNEL(c, SERV, c->nick, 
			       irc_room_encode(c->atmp, room));
}

void
irc_out_room_full(connection_t *c, char *room)
{
	IRC_SEND_CHANNELISFULL(c, SERV, c->nick, 
			       irc_room_encode(c->atmp, room));
}

void
irc_out_room_alrexist(connection_t *c, char *room)
{
	IRC_CONSOLE(c, "la stanza %s esiste gia'", 
		    irc_room_encode(c->atmp, room));
}

void
irc_out_foundusers(connection_t *c, unsigned int argc, char **argv)
{
	unsigned int i;

	if (argc == 0) {
		IRC_SEARCHMSG(c,
			      "nessun utente trovato con la ricerca effettuata");
		return;
	}

	for (i = 0; i < argc; i++)
		if (strcmp(argv[i], c->nick))
			IRC_SEND_JOIN(c, argv[i], argv[i], SERV, SEARCHUSR);
}

void
irc_out_selnick(connection_t *c, unsigned int nickc, char **nickv, char **passv)
{
	unsigned int i;
	char nick_str[2048], *pass = NULL;

	nick_str[0] = '\0';

	if (nickc == 0) {
		strlcpy(c->nick, c->user, sizeof(c->nick));

		EMIT(c, "send login", c->nick, c->pass);
		return;
	}

	for (i = 0; i < nickc; i++) {
		strlcat(nick_str, " ", sizeof(nick_str));
		strlcat(nick_str, nickv[i], sizeof(nick_str));

		if (pass == NULL && !strcmp(nickv[i], c->nick))
			pass = passv[i];
	}

	IRC_CONSOLE(c, "nicks:%s", nick_str);

	PDEBUG("sening login %s %s\n", c->nick, pass);
	EMIT(c, "send login", c->nick, pass);
}
