
/* $Id: client.c,v 1.1 2005/04/16 09:23:22 benkj Exp $ */

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
#include "c6/c6.h"
#include "error.h"
#include "socket.h"

static void
c6_send(connection_t *c, buffer_t *payload)
{
	buffer_t *pkt;
	size_t head_len;
	struct c6_pkt_head *pl_head = (struct c6_pkt_head *)payload->data;

	pkt = buf_new_pack(c->atmp, "\x10\x0F%^hu%^hu",
			   C6(c)->clt_id, payload->size);
	head_len = pkt->size;

	buf_realloc(&pkt, head_len + payload->size);

	pl_head->len = htons(payload->size - sizeof(struct c6_pkt_head));
	c6_packet_encode(pkt->data + head_len, payload->data, payload->size,
			 C6(c)->reordered_key);

	if (!ATOMICIO(write, C6(c)->fd, pkt->data, pkt->size))
		EMIT(c, "gui fatal", "errore in scrittura");

	C6(c)->clt_id++;
}

#define C6_EMULE_VERS	0x022d

void
c6_login(connection_t *c, char *user, char *pass)
{
	char cuser[16], cpass[16], xxx_pass[9];
	buffer_t *buf;

	c6_nick_encode(user, cuser, C6(c)->server_key);

	/* XXX: sembra che il server abbia problemi con pass 
	 *      più lunghe di 8 caratteri */
	memcpy(xxx_pass, pass, MIN(8, strlen(pass) + 1));
	xxx_pass[8] = '\0';

	c6_pass_encode(xxx_pass, cpass, C6(c)->server_key);

	buf = buf_new_pack(c->atmp,
			   "\x10\x01%^hu%^hu%bu%s%bu%16p%bu%16p%^hu%u%Z\x14",
			   C6(c)->clt_id, 0, (uint8_t)strlen(user),
			   user, (uint8_t)16, cpass, (uint8_t)16, cuser,
			   C6(c)->ip.s_addr, C6_EMULE_VERS);

	c6_send(c, buf);
}

void
c6_send_pong(connection_t *c)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp, "\x10\x0b%^hu%^hu", C6(c)->clt_id, 0);

	c6_send(c, buf);
}

void
c6_get_style(connection_t *c, unsigned int *elms, char ***desc)
{
	*elms = 13;

	*desc = a_nnew(c->atmp, 13, char *);

	(*desc)[0] = "Normale";
	(*desc)[1] = "Corsivo";
	(*desc)[2] = "Normale";
	(*desc)[3] = "Grassetto";
	(*desc)[4] = "Arancione grassetto";
	(*desc)[5] = "Corsivo";
	(*desc)[6] = "Verde";
	(*desc)[7] = "Rosso grassetto";
	(*desc)[8] = "Corsivo con sfondo verde";
	(*desc)[9] = "Sfondo verde";
	(*desc)[10] = "Rosso grassetto sfondo verde";
	(*desc)[11] = "Rosso grassetto (anche il nick)";
	(*desc)[12] = "Rosso corsivo (anche il nick) ";
}

void
c6_set_style(connection_t *c, unsigned int *style)
{
	if (*style != -1U && *style <= 12) {
		C6(c)->style = (uint16_t)*style;
		*style = 1;
	} else
		*style = 0;
}

void
c6_send_msg(connection_t *c, char *to, char *msg)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp,
			   "\x10\x08%^hu%^hu%bu%s%Z\01%bu%s%^hu%^hu%s",
			   C6(c)->clt_id, 0,
			   (uint8_t)strlen(c->nick), c->nick,
			   (uint8_t)strlen(to), to,
			   (uint16_t)(strlen(msg) + sizeof(uint16_t)),
			   C6(c)->style, msg);

	c6_send(c, buf);
}

void
c6_send_ofmsg(connection_t *c, char *to, char *msg)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp,
			   "\x10\x09%^hu%^hu%bu%s%Z\01%bu%s%^hu%s",
			   C6(c)->clt_id, 0,
			   (uint8_t)strlen(c->nick), c->nick,
			   (uint8_t)strlen(to), to, (uint16_t)strlen(msg), msg);

	c6_send(c, buf);
}


static void
reqdel_users(connection_t *c, int argc, char **argv, uint8_t cmd)
{
	buffer_t *buf;
	int i;

	buf = buf_new_pack(c->atmp, "\x10%bu%^hu%^hu%^hu%bu%s",
			   cmd, C6(c)->clt_id, 0, argc,
			   (uint8_t)strlen(argv[0]), argv[0]);

	for (i = 1; i < argc; i++)
		buf_append_pack(&buf, "%bu%s",
				(uint8_t)strlen(argv[i]), argv[i]);

	c6_send(c, buf);
}

void
c6_req_users(connection_t *c, int argc, char **argv)
{
	reqdel_users(c, argc, argv, 0x03);
}

void
c6_del_users(connection_t *c, int argc, char **argv)
{
	reqdel_users(c, argc, argv, 0x04);
}

void
c6_send_status(connection_t *c)
{
	buffer_t *buf;
	uint8_t status = 0;

	/* *INDENT-OFF* */
	CDEBUG(
		if (c->status & STATUS_AVAILABLE)
	    		PDEBUG("stato normale"); 
		if (c->status & STATUS_BUSY)
			PDEBUG("stato occupato"); 
		if (c->status & STATUS_NETFRIENDS)
	   		PDEBUG("stato solo amici");
	);
	/* *INDENT-ON* */

	if (c->status & STATUS_AVAILABLE)
		status |= C6_STATUS_AVAILABLE;
	if (c->status & STATUS_BUSY)
		status |= C6_STATUS_BUSY;
	if (c->status & STATUS_NETFRIENDS)
		status |= C6_STATUS_NETFRIENDS;
	if (c->status & STATUS_ONLINE)
		status |= C6_STATUS_ONLINE;
	if (c->status & STATUS_VISIBLE_IP)
		status |= C6_STATUS_VISIBLE_IP;

	buf = buf_new_pack(c->atmp, "\x10\x0A%^hu%^hu%bu",
			   C6(c)->clt_id, 0, status);

	c6_send(c, buf);
}

void
c6_req_userinfo(connection_t *c, char *nick)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp, "\x10\x0C%^hu%^hu%bu%s",
			   C6(c)->clt_id, 0, (uint8_t)strlen(nick), nick);

	c6_send(c, buf);
}

void
c6_req_roomlist(connection_t *c)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp, "\x10\x24%^hu%^hu%bu%s",
			   C6(c)->clt_id, 0, (uint8_t)strlen(c->nick), c->nick);

	c6_send(c, buf);
}

void
c6_req_roominfo(connection_t *c, char *room)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp, "\x10\x26%^hu%^hu%bu%s",
			   C6(c)->clt_id, 0, (uint8_t)strlen(room), room);

	c6_send(c, buf);
}

static void
room_leave_enter(connection_t *c, char *room, uint8_t cmd)
{
	buffer_t *buf;
	struct proom_entry *pre;

	buf = buf_new_pack(c->atmp, "\x10%bu%^hu%^hu%bu%s%bu%s",
			   cmd, C6(c)->clt_id, 0, (uint8_t)strlen(c->nick),
			   c->nick, (uint8_t)strlen(room), room);

	c6_send(c, buf);

	pre = a_new(c->area, struct proom_entry);
	pre->room = a_strdup(c->area, room);

	SIMPLEQ_INSERT_TAIL(&C6(c)->proom, pre, entry);
}

void
c6_room_enter(connection_t *c, char *room)
{
	room_leave_enter(c, room, 0x21);
}

void
c6_room_leave(connection_t *c, char *room)
{
	room_leave_enter(c, room, 0x22);
}

void
c6_send_roommsg(connection_t *c, char *to, char *msg)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp,
			   "\x10\x20%^hu%^hu%bu%s%bu%s%^hu%^hu%s",
			   C6(c)->clt_id, 0,
			   (uint8_t)strlen(c->nick), c->nick,
			   (uint8_t)strlen(to), to,
			   (uint16_t)(strlen(msg) + sizeof(uint16_t)),
			   C6(c)->style, msg);

	c6_send(c, buf);
}

void
c6_create_room(connection_t *c, char *room, char pub, char *topic)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp,
			   "\x10%%%^hu%^hu%bu%s%bu%s%bu%s%bu%Z%Z",
			   C6(c)->clt_id, 0,
			   (uint8_t)strlen(c->nick), c->nick,
			   (uint8_t)strlen(room), room,
			   (uint8_t)strlen(topic), topic, pub ? 01 : 00);

	c6_send(c, buf);
}

void
c6_ufind_by_email(connection_t *c, char *email)
{
	buffer_t *buf;

	buf = buf_new_pack(c->atmp, "\x10\x05%^hu%^hu%bu%s",
			   C6(c)->clt_id, 0, (uint8_t)strlen(email), email);

	c6_send(c, buf);
}

void
c6_search_nicks(connection_t *c)
{
	buffer_t *buf, *payload;
	uint16_t num_val = 0;
	size_t i;

	payload = buf_new_dummy(c->atmp);

	for (i = 0; i < 256 && i < c6_userinfo_elms; i++) {
		if (C6(c)->search_id[i] == 0xff)
			continue;

		buf_append_pack(&payload, "%bu%bu",
				(i + 1) & 0xff, C6(c)->search_id[i]);
		num_val++;
	}

	buf = buf_new_pack(c->atmp, "\x10\x0E%^hu%^hu%^hu%r",
			   C6(c)->clt_id, 0, num_val, payload);

	c6_send(c, buf);
}

void
c6_send_username(connection_t *c, char *user, char *pass)
{
	buffer_t *buf, *str, *blob;
	char nick[] = "nicksforuser";

	str = buf_new_pack(c->atmp, "%s%s%s%s%s",
			   "<user>\n  <username>", user,
			   "</username>\n  <password>", pass,
			   "</password>\n</user>\n");

	blob = buf_new(c->atmp, NULL, ((str->size / 8) + 1) * 8);

	memcpy(blob->data, str->data, str->size);
	PDEBUG("blob-data = %s", blob->data);
	c6_encode_xml_blob(blob->data, blob->size, C6(c)->md5_key);

	buf =
	    buf_new_pack(c->atmp, "\x10\x31%^hu%^hu%bu%s%^hu%r\x02\x2d",
			 C6(c)->clt_id, 0, (uint8_t)strlen(nick), nick,
			 (uint16_t)blob->size, blob);

	c6_send(c, buf);
}
