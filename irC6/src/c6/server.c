
/* $Id: server.c,v 1.1 2005/04/16 09:23:23 benkj Exp $ */

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
#include "socket.h"
#include "error.h"
#include "version.h"
#include "str_missing.h"

#include "c6/c6.h"

#define CHECK_BAD(a, op, b, name)					\
if ((size_t)(a) op (size_t)(b))	{					\
	PDEBUG("bad " #name " : %i %i\n", (a), (b));			\
	return;								\
}

#define _CER(_func, _rd)						\
do {									\
	PDEBUG("ricevuto " #_func);					\
	if ((_rd) == 0) {						\
		EMIT(c, "gui warning",		 			\
			 "ricevuto un pacchetto non valido: " #_func);	\
		PDEBUG("**INAVALID** packet in " #_func);		\
		EMIT(c, "gui dump data", buf, len);			\
		return;							\
	}								\
} while (0)

#define C6_UNPACK(_c, _buf, _len, _fmt, _args...)			\
	buf_unpack_data((_c)->atmp, _buf, _len, _fmt , ##_args)


static void
cmd_helo(connection_t *c, void *buf, size_t len)
{
	char *key;
	buffer_t *tmp;

	_CER(HELO, C6_UNPACK(c, buf, len, "%Z\x02\x08%8p", &key));

	memcpy(C6(c)->server_key, key, 8);
	c6_reorder_key_server(C6(c)->server_key, C6(c)->reordered_key);

	tmp = buf_new_pack(c->atmp, "%8p%10p%Z", C6(c)->server_key,
			   "\x4a\x4e\x42\x55\x25\x5e\x26\x43\x29\x25");
	c6_set_md5_key(C6(c)->md5_key, tmp->data);

	c6_send_username(c, c->user, c->pass);
}

static void
cmd_redir(connection_t *c, void *buf, size_t len)
{
	struct in_addr *ip;
	uint16_t port;

	_CER(REDIR, C6_UNPACK(c, buf, len, "%4p%2k%hu", &ip, &port));

	memcpy(&C6(c)->srv_addr.sin_addr, ip, sizeof(struct in_addr));
	C6(c)->srv_addr.sin_port = port;

	PDEBUG("mi devo connettere a %s %i",
	       inet_ntoa(C6(c)->srv_addr.sin_addr),
	       ntohs(C6(c)->srv_addr.sin_port));

	event_del(&C6(c)->ev);
	close(C6(c)->fd);

	C6(c)->fd = connect_addr(&C6(c)->srv_addr);

	if (C6(c)->fd == -1)
		EMIT(c, "gui fatal", "redirect fallito");

	if (!get_my_ip(C6(c)->fd, &C6(c)->ip))
		EMIT(c, "gui fatal",
		     "impossibile ottenere il proprio indirizzo");

	C6(c)->srv_id = C6(c)->clt_id = 1;
	memset(C6(c)->server_key, 0, sizeof(C6(c)->server_key));
	memset(C6(c)->reordered_key, 0, sizeof(C6(c)->reordered_key));

	PDEBUG("FATTO REDIRECT");

	event_set(&C6(c)->ev, C6(c)->fd, EV_READ | EV_PERSIST, c6_read, c);
	event_add(&C6(c)->ev, NULL);
}

static void
cmd_wlcome(connection_t *c, void *buf, size_t len)
{
	char *msg;

	c6_send_status(c);

	_CER(WLCOME, C6_UNPACK(c, buf, len, "%^h$%s", &msg));

	EMIT(c, "gui welcome", msg);
}

static void
cmd_ping(connection_t *c, void *buf, size_t len)
{
	c6_send_pong(c);
}

static void
cmd_nouser(connection_t *c, void *buf, size_t len)
{
	EMIT(c, "gui invalid user", c->nick);
}

static void
cmd_invpass(connection_t *c, void *buf, size_t len)
{
	EMIT(c, "gui invalid pass", c->nick);
}

static void
cmd_alrcon(connection_t *c, void *buf, size_t len)
{
	EMIT(c, "gui nick already connected");
}

static void
cmd_infologin(connection_t *c, void *_buf, size_t len)
{
	size_t offs, ret;
	uint8_t *buf = (uint8_t *)_buf;
	uint8_t i, banners, pulsanti;
	char **bannerv, **pulsv, tmp[2048];

	offs = C6_UNPACK(c, buf, len, "%4k%bu", &banners);
	_CER(INFOLOGIN, offs);

	bannerv = a_nnew(c->atmp, banners, char *);

	EMIT(c, "gui login done", &C6(c)->srv_addr);

	for (i = 0; i < banners; i++) {
		char *gif, *link, *nome;

		ret = C6_UNPACK(c, buf + offs, len - offs,
				"%b$%s%b$%s%b$%s", &gif, &link, &nome);
		_CER(INFOLOGIN_BANNER, ret);

		offs += ret;

		snprintf(tmp, sizeof(tmp),
			 "banner: %s -> %s -> %s", nome, link, gif);
		bannerv[i] = a_strdup(c->atmp, tmp);
	}

	EMIT(c, "gui banners", (unsigned int)banners, bannerv);

	ret = C6_UNPACK(c, buf + offs, len - offs, "%2k%bu", &pulsanti);
	_CER(INFOLOGIN2, ret);

	pulsv = a_nnew(c->atmp, pulsanti, char *);

	offs += ret;

	for (i = 0; i < pulsanti; i++) {
		char id, *link, *nome;

		ret = C6_UNPACK(c, buf + offs, len - offs,
				"%1k%bu%b$%s%b$%s", &id, &link, &nome);
		_CER(INFOLOGIN_BOTTON, ret);

		offs += ret;

		snprintf(tmp, sizeof(tmp),
			 "botton: %s |> id: %i url: %s", nome, (int)id, link);
		pulsv[i] = a_strdup(c->atmp, tmp);
	}

	EMIT(c, "gui bottons", (unsigned int)pulsanti, pulsv);
}

static char *colors_str[13] = {
	"",			/* Normale */
	"\037",			/* Corsivo */
	"",			/* Normale */
	"\002",			/* Grassetto */
	"\002\00307",		/* Arancione grassetto */
	"\037",			/* Corsivo */
	"\00309",		/* Verde */
	"\002\00304",		/* Rosso grassetto */
	"\037\003,09",		/* Corsivo con sfondo verde */
	"\003,09",		/* Sfondo verde */
	"\002\0034,09",		/* Rosso grassetto sfondo verde */
	"\002\00304",		/* Rosso grassetto */
	"\037\00304"		/* Rosso corsivo */
};

static void
cmd_message(connection_t *c, void *buf, size_t len)
{
	char *from, *to, *msg;
	buffer_t *tmpmsg;
	uint16_t style, *tmp;
	size_t size;

	_CER(MESSAGE, C6_UNPACK(c, buf, len, "%b$%s%b$%s%^h$%r", &from,
				&to, &tmpmsg));

	if (tmpmsg->size <= sizeof(uint16_t))
		_CER(MESSAGE_SIZE, 0);

	size = tmpmsg->size + 1;
	buf_realloc(&tmpmsg, size);

	tmp = (uint16_t *)tmpmsg->data;
	msg = (char *)(tmp + 1);
	style = ntohs(*tmp);

	EMIT(c, "gui message", from, to, msg,
	     (style <= 12) ? colors_str[style] : "");
}

static void
cmd_errpkt(connection_t *c, void *buf, size_t len)
{
	uint16_t *count = (uint16_t *)buf;

	CHECK_BAD(len, <, sizeof(uint16_t), payload_len);

	PDEBUG("messaggio %hu non valido", ntohs(*count));
	EMIT(c, "gui packet error", (unsigned int)ntohs(*count));
}

static void
cmd_encoded(connection_t *c, void *buf, size_t len)
{
	char *buf2;

	buf2 = a_alloc(c->atmp, len);

	c6_packet_encode(buf2, buf, len, C6(c)->reordered_key);

	c6_process_input(c, buf2, len);
}

static void
cmd_snd_users(connection_t *c, void *_buf, size_t len)
{
	char *nick, **userv;
	uint8_t *buf = (uint8_t *)_buf;
	uint16_t i, users, *max = (uint16_t *)_buf;
	size_t offs = sizeof(uint16_t), ret;

	CHECK_BAD(len, <, sizeof(uint16_t), payload_len);

	users = ntohs(*max);
	userv = a_nnew(c->atmp, users, char *);

	for (i = 0; i < ntohs(*max); i++) {
		ret = C6_UNPACK(c, buf + offs, len - offs, "%b$%s", &nick);
		_CER(SEND_USERS, ret);

		userv[i] = nick;

		offs += ret;
	}

	EMIT(c, "gui got users", (unsigned int)users, userv);
}

static void
cmd_status_user(connection_t *c, void *buf, size_t len)
{
	char *nick, type;

	PDEBUG("ricevuto STATUS_USER");

	_CER(STATUS_USER, C6_UNPACK(c, buf, len, "%b$%s%bu", &nick, &type));

	switch (type) {
	case 1:
		EMIT(c, "gui user online", nick);
		break;
	case 0:
		EMIT(c, "gui user offline", nick);
		break;
	default:
		EMIT_FMT(c, "gui warning",
			 "status del nick %s sconosciuto %d", nick, type);
	}
}

static void
cmd_userchgstatus(connection_t *c, void *buf, size_t len)
{
	uint8_t c6_status;
	unsigned int status;
	char *nick;

	PDEBUG("ricevuto STATUS_USER_CHG");

	_CER(STATUS_USER_CHG, C6_UNPACK(c, buf, len, "%b$%s%9k%bu", 
			    		&nick, &c6_status));

	status = 0;
	
	if (c6_status & C6_STATUS_AVAILABLE)
		status |= STATUS_AVAILABLE;
	if (c6_status & C6_STATUS_BUSY)
		status |= STATUS_BUSY;
	if (c6_status & C6_STATUS_NETFRIENDS)
		status |= STATUS_NETFRIENDS;
	if (c6_status & C6_STATUS_ONLINE)
		status |= STATUS_ONLINE;
	if (c6_status & C6_STATUS_VISIBLE_IP)
		status |= STATUS_VISIBLE_IP;

	EMIT(c, "gui user change status", nick, status);
}

static void
cmd_user_info(connection_t *c, void *_buf, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	char *nick, *status_str, **info;
	uint32_t otime;
	struct in_addr *ip;
	uint8_t status, id, val;
	uint16_t entries, i;
	time_t online_time;
	struct tm ltime;
	size_t offs, ret;
	unsigned int infos;

	offs = C6_UNPACK(c, buf, len, "%b$%s%^u%4p%bu%^hu", &nick,
			 &otime, &ip, &status, &entries);
	_CER(USERINFO, offs);

	online_time = (time_t)otime;
	/* gmtime_r(&online_time, &ltime); */
	localtime_r(&online_time, &ltime);

	if (C6_IS_ONLINE(status)) {
		status_str = (status & C6_STATUS_AVAILABLE) ?
		    "Disponibile" : (status & C6_STATUS_NETFRIENDS) ?
		    "Solo Netfriend" : "Impegnato";
	} else {
		ip = NULL;
		status_str = "Non connesso";
	}

	info = a_nnew(c->atmp, entries * 2, char *);

	infos = 0;

	for (i = 0; i < entries; i++) {
		ret = C6_UNPACK(c, buf + offs, len - offs, "%bu%bu", &id, &val);
		_CER(USERINFO_ENTRIES, ret);

		offs += ret;

		if (id == 0 || id > c6_userinfo_elms ||
		    val >= c6_userinfo[id - 1].entries)
			continue;

		info[infos * 2] = (char *)c6_userinfo[id - 1].name;
		info[(infos * 2) + 1] = (char *)c6_userinfo[id - 1].info[val];

		infos++;
	}

	EMIT(c, "gui user info", nick, status_str, &ltime, ip, infos, info);
}

static void
cmd_exit_ok(connection_t *c, void *buf, size_t len)
{
	EMIT(c, "gui quit", "Disconnessione dal server C6");
	irC6_close_connection(c);
}

static void
cmd_roomlist(connection_t *c, void *_buf, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	size_t offs, ret;
	uint16_t i, rooms, users;
	char *room, type, **roomv;
	unsigned int *usersv, *pubv;

	offs = C6_UNPACK(c, buf, len, "%Z%Z%^hu", &rooms);
	_CER(ROOMLIST, offs);

	roomv = a_nnew(c->atmp, rooms, char *);
	usersv = a_nnew(c->atmp, rooms, unsigned int);
	pubv = a_nnew0(c->atmp, rooms, unsigned int);

	for (i = 0; i < rooms; i++) {
		ret = C6_UNPACK(c, buf + offs, len - offs,
				"%b$%s%^hu%bu", &room, &users, &type);

		_CER(ROOMLIST2, ret);

		offs += ret + 2;

		roomv[i] = room;
		usersv[i] = (unsigned int)users;

		switch (type & 03) {
		case 1:
		case 3:
			pubv[i] = 1;
			break;
		default:
			pubv[i] = 0;
		}
	}

	EMIT(c, "gui room list", (unsigned int)rooms, roomv, usersv, pubv);
}

static void
cmd_roominfo(connection_t *c, void *buf, size_t len)
{
	char *room, chtype, *topic, *founder;
	uint16_t users;
	unsigned int pub;

	_CER(ROOMINFO, C6_UNPACK(c, buf, len,
				 "%b$%s%bu%b$%s%b$%s%^hu", &room, &chtype,
				 &topic, &founder, &users));

	switch (chtype) {
	case 0xd:
	case 0x2b:
		pub = 1;
		break;
	default:
		pub = 0;
	}

	EMIT(c, "gui room info", room, topic, founder,
	     (unsigned int)users, pub);
}

static char *
_get_last_room(connection_t *c)
{
	struct proom_entry *pre;
	char *room;

	if ((pre = SIMPLEQ_FIRST(&C6(c)->proom)) == NULL)
		return NULL;

	room = pre->room;
	SIMPLEQ_REMOVE_HEAD(&C6(c)->proom, pre, entry);

	PDEBUG("room pop %s", room);
	
	a_free(c->area, pre);
	area_relink(c->area, c->atmp, room);

	return room;
}

static void
cmd_roomuserlist(connection_t *c, void *_buf, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	char *room, *nick, **userv;
	size_t offs, ret;
	uint8_t nicks, i;

	PDEBUG("room userlist");
	
	_get_last_room(c);

	offs = C6_UNPACK(c, buf, len, "%b$%s%2k%bu", &room, &nicks);
	_CER(ROOMUSERLIST, offs);

	userv = a_nnew(c->atmp, nicks, char *);

	for (i = 0; i < nicks; i++) {
		ret = C6_UNPACK(c, buf + offs, len - offs, "%b$%s", &nick);
		_CER(ROOMUSERLIST_NICK, ret);

		userv[i] = nick;
		offs += ret;
	}

	EMIT(c, "gui room user list", room, (unsigned int)nicks, userv);
}

static void
cmd_roommsg(connection_t *c, void *buf, size_t len)
{
	char *from, *to, *room, *msg, type;
	buffer_t *tmpmsg;
	uint16_t style, *tmp;
	size_t size;

	_CER(ROOM_MESSAGE, C6_UNPACK(c, buf, len,
				     "%b$%s%b$%s%b$%s%bu%^h$%r", &from, &room,
				     &to, &type, &tmpmsg));

	if (tmpmsg->size <= sizeof(uint16_t))
		_CER(ROOM_MESSAGE_SIZE, 0);

	size = tmpmsg->size + 1;
	buf_realloc(&tmpmsg, size);

	tmp = (uint16_t *)tmpmsg->data;
	msg = (char *)(tmp + 1);
	style = ntohs(*tmp);

	EMIT(c, "gui room message", room, from, to, msg,
	     (style <= 12) ? colors_str[style] : "");
}

static void
cmd_roomuenter(connection_t *c, void *buf, size_t len)
{
	char *nick, *room;

	_CER(ROOM_USERENTER, C6_UNPACK(c, buf, len, "%b$%s%b$%s",
				       &nick, &room));

	EMIT(c, "gui room user enter", room, nick);
}

static void
cmd_roomuleave(connection_t *c, void *buf, size_t len)
{
	char *nick, *room;

	_CER(ROOM_USERENTER, C6_UNPACK(c, buf, len, "%b$%s%b$%s",
				       &nick, &room));

	EMIT(c, "gui room user leave", room, nick);
}

static void
cmd_roomcanleave(connection_t *c, void *buf, size_t len)
{
	char *room = _get_last_room(c);

	if (room)
		EMIT(c, "gui room can leave", room);
}

static void
cmd_roomnotexist(connection_t *c, void *buf, size_t len)
{
	char *room = _get_last_room(c);

	if (room)
		EMIT(c, "gui room not exist", room);
}

static void
cmd_roomisfull(connection_t *c, void *buf, size_t len)
{
	char *room;

	_get_last_room(c);
	_CER(ROOM_ISFULL, C6_UNPACK(c, buf, len, "%b$%s%1k", &room));

	EMIT(c, "gui room is full", room);
}

static void
cmd_roomalrexist(connection_t *c, void *buf, size_t len)
{
	/* FIXME: mettere il nome della stanza */
	EMIT(c, "gui room already exist", "");
}

static void
cmd_foundnicks(connection_t *c, void *_buf, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	uint16_t i, num_user, *tmp = (uint16_t *)_buf;
	size_t ret, offs = sizeof(uint16_t);
	char *nick, status, **userv = NULL;

	CHECK_BAD(len, <, sizeof(uint16_t), foundnicks);
	num_user = ntohs(*tmp);

	if (num_user != 0) {
		userv = a_nnew(c->atmp, num_user, char *);

		for (i = 0; i < num_user; i++) {
			ret = C6_UNPACK(c, buf + offs, len - offs,
					"%b$%s%bu", &nick, &status);

			_CER(foundnicks, ret);

			offs += ret;

			userv[i] = nick;
		}
	}

	EMIT(c, "gui found users", (unsigned int)num_user, userv);
}

static void
cmd_foundemail(connection_t *c, void *_buf, size_t len)
{
	uint8_t *buf = (uint8_t *)_buf;
	uint16_t i, num_user, *tmp = (uint16_t *)_buf;
	size_t ret, offs = sizeof(uint16_t);
	char *nick, **userv = NULL;

	CHECK_BAD(len, <, sizeof(uint16_t), foundemail);
	num_user = ntohs(*tmp);

	if (num_user != 0) {
		userv = a_nnew(c->atmp, num_user, char *);

		for (i = 0; i < num_user; i++) {
			ret = C6_UNPACK(c, buf + offs, len - offs,
					"%b$%s", &nick);

			_CER(foundemail, ret);

			offs += ret;

			userv[i] = nick;
		}
	}

	EMIT(c, "gui found users", (unsigned int)num_user, userv);
}

#define NLIST_ASSERT(_expr)	do {					\
	if ((_expr) == NULL) 						\
		EMIT(c, "gui fatal", "errore nella ricezione dei nick");\
} while (0)

static void
cmd_nicks(connection_t *c, void *buf, size_t len)
{
	uint16_t *tmp = (uint16_t *)buf;
	buffer_t *blob;
	char *xml, *p, *q, **nickv, **passv;
	unsigned int nicks;

	if (len <= sizeof(uint16_t) || *tmp == 0)
		EMIT(c, "gui fatal", "errore nella ricezione dei nick");

	_CER(NICKS, C6_UNPACK(c, buf, len, "%^h$%r", &blob));
	c6_decode_xml_blob(blob->data, blob->size, C6(c)->md5_key);
	xml = blob->data;

	if (xml[blob->size - 1])
		xml[blob->size - 1] = '\0';

	PDEBUG("xml str: %s", xml);

	if (strstr(xml, "<oldnick")) {
		EMIT(c, "gui select nick", 0, NULL, NULL);
		return;
	}

	if ((p = strstr(xml, "<error>")) != NULL) {
		NLIST_ASSERT(q = strchr(p + 1, '<'));
		p = q + 1;

		NLIST_ASSERT(q = strpbrk(p, "</>"));
		*q = '\0';

		EMIT(c, "gui fatal", "errore nella ricezione dei nick");
	}

	nickv = NULL;
	passv = NULL;

	for (nicks = 0, nickv = NULL, passv = NULL;; nicks++) {
		PDEBUG("wawa - %s\n", xml);
		if ((p = strstr(xml, "<nickname>")) == NULL)
			break;
		p += strlen("<nickname>");

		NLIST_ASSERT(q = strchr(p, '<'));
		*q = '\0';

#undef roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

		if ((nicks % 10) == 0) {
			a_nexpand(c->atmp, &nickv,
				  roundup(nicks + 1, 10), sizeof(char *));
			a_nexpand(c->atmp, &passv,
				  roundup(nicks + 1, 10), sizeof(char *));
		}

		nickv[nicks] = p;

		NLIST_ASSERT(p = strstr(q + 1, "<password>"));
		p += strlen("<password>");

		NLIST_ASSERT(q = strchr(p, '<'));
		*q = '\0';

		passv[nicks] = p;

		PDEBUG(" NICK %s %s", nickv[nicks], passv[nicks]);
		
		xml = q + 1;
	}

	EMIT(c, "gui select nick", nicks, nickv, passv);
}

/* *INDENT-OFF* */

enum {
	CMD_INFOLOGIN = 0x1,
	CMD_REDIR,
	CMD_INVPASS,
	CMD_ALRCON,
	CMD_EXIT_OK,
	CMD_SND_USERS,
	CMD_FOUND_BY_EMAIL,
	CMD_STATUS_USER = 0xa,
	CMD_ERRPKT = 0xd,
	CMD_NOUSER,
	CMD_MESSAGE,
	CMD_WLCOME = 0x10,
	CMD_PING,
	CMD_HELO,
	CMD_USER_INFO = 0x14,
	CMD_FOUND_NICKS = 0x16,
	CMD_ENCODED = 0x18,
	CMD_USER_CHG_STATUS = 0x1c,
	CMD_ROOM_MESSAGE = 0x30,
	CMD_ROOM_NOTEXIST,
	CMD_ROOM_USERLIST,
	CMD_ROOM_USERENTER,
	CMD_ROOM_USERLEAVE,
	CMD_ROOM_CANLEAVE,
	CMD_ROOM_INFO = 0x37,
	CMD_ROOM_ALREX = 0x39,
	CMD_ROOM_FULL = 0x3b,
	CMD_ROOM_LIST,
	CMD_NICKS = 0x45
};

static void
(*(cmd_table)[256]) (connection_t *, void *, size_t) = {
	[CMD_HELO] cmd_helo,
	[CMD_REDIR] cmd_redir,
	[CMD_INFOLOGIN] cmd_infologin,
	[CMD_WLCOME] cmd_wlcome,
	[CMD_PING] cmd_ping,
	[CMD_NOUSER] cmd_nouser,
	[CMD_INVPASS] cmd_invpass,
	[CMD_ALRCON] cmd_alrcon,
	[CMD_MESSAGE] cmd_message,
	[CMD_ENCODED] cmd_encoded,
	[CMD_ERRPKT] cmd_errpkt,
	[CMD_SND_USERS] cmd_snd_users,
	[CMD_STATUS_USER] cmd_status_user,
	[CMD_USER_INFO] cmd_user_info,
	[CMD_EXIT_OK] cmd_exit_ok,
	[CMD_ROOM_LIST] cmd_roomlist,
	[CMD_ROOM_INFO] cmd_roominfo,
	[CMD_ROOM_USERLIST] cmd_roomuserlist,
	[CMD_ROOM_MESSAGE] cmd_roommsg,
	[CMD_ROOM_USERENTER] cmd_roomuenter,
	[CMD_ROOM_USERLEAVE] cmd_roomuleave,
	[CMD_ROOM_CANLEAVE] cmd_roomcanleave,
	[CMD_ROOM_NOTEXIST] cmd_roomnotexist,
	[CMD_ROOM_FULL] cmd_roomisfull,
	[CMD_ROOM_ALREX] cmd_roomalrexist,
	[CMD_FOUND_NICKS] cmd_foundnicks,
	[CMD_FOUND_BY_EMAIL] cmd_foundemail,
	[CMD_USER_CHG_STATUS] cmd_userchgstatus,
	[CMD_NICKS] cmd_nicks
};

/* *INDENT-ON* */

void
c6_process_input(connection_t *c, char *buf, size_t len)
{
	struct c6_pkt_head *sp = (struct c6_pkt_head *)buf;

	PDEBUG("user count %hu @ server count  %hu", ntohs(sp->id),
	       C6(c)->srv_id);

	/* FIXME: non sembra che il server si preoccupi di inviare
	 * contatori corretti... */
	/* CHECK_BAD(ntohs(sp->id), !=, C6(c)->srv_id, srv_id); */
	/* CHECK_BAD(ntohs(sp->len), !=, (uint16_t) len, len); */

	C6(c)->srv_id++;

	if (cmd_table[sp->cmd])
		(*cmd_table[sp->cmd]) (c, buf + sizeof(struct c6_pkt_head),
				       len - sizeof(struct c6_pkt_head));
	else {
		EMIT_FMT(c, "gui warning",
			 "comando del server sconosciuto: %#2x",
			 (uint8_t)sp->cmd);

		EMIT(c, "gui dump data", buf + sizeof(struct c6_pkt_head), len);
	}
}
