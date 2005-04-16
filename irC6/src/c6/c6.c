
/* $Id: c6.c,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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
#include "c6/marshal.h"

static void
resolv_c6_host(connection_t *c, char *host, char *port)
{
	struct sockaddr_in srv_addr;

	if (!resolv(host, port, &srv_addr)) {
#ifdef __GNU_LIBRARY__
		/* chiude la struttura interna di gethostbyname() */
		if (_res.options & RES_INIT) {
			res_nclose(&_res);
			_res.options = RES_DEFAULT;
		}

		if (!resolv(host, port, &srv_addr))
#endif
			EMIT_FMT(c, "gui fatal",
				 "%s:%s host del server C6"
				 " sconosciuto o non valido", host, port);
	}

	memcpy(&C6(c)->srv_addr, &srv_addr, sizeof(struct sockaddr_in));
}

static void
c6_connect(connection_t *c, char *host, char *port)
{
	resolv_c6_host(c, host, port);

	C6(c)->fd = connect_addr(&C6(c)->srv_addr);
	if (C6(c)->fd == -1)
		EMIT(c, "gui fatal", "impossibile connettersi al server C6");

	if (!get_my_ip(C6(c)->fd, &C6(c)->ip))
		EMIT(c, "gui fatal",
		     "impossibile ottenere il proprio indirizzo");

	event_set(&C6(c)->ev, C6(c)->fd, EV_READ | EV_PERSIST, c6_read, c);

	event_add(&C6(c)->ev, NULL);
}


#define _C6EV(name, cb, msl)						\
	{ name, (marshal_callback_t)cb, msl, 0}


/* *INDENT-OFF* */
static struct {
	char *name;
	marshal_callback_t cb;
	marshal_func_t marshal;
	unsigned int id;
} c6_ev[] = {
	_C6EV("connect", c6_connect, c6msl_conn_charp_charp),
	_C6EV("send login", c6_login, c6msl_conn_charp_charp),
	_C6EV("send username", c6_send_username, c6msl_conn_charp_charp),
	_C6EV("send pong", c6_send_pong, c6msl_conn),
	_C6EV("send message", c6_send_msg, c6msl_conn_charp_charp),
	_C6EV("send offline message", c6_send_ofmsg, c6msl_conn_charp_charp),
	_C6EV("send users req", c6_req_users, c6msl_conn_int_charpp),
	_C6EV("send users del", c6_del_users, c6msl_conn_int_charpp),
	_C6EV("send status", c6_send_status, c6msl_conn),
	_C6EV("get styles", c6_set_style, c6msl_conn_uintp_charppp),
	_C6EV("set style", c6_set_style, c6msl_conn_uintp),
	_C6EV("send userinfo req", c6_req_userinfo, c6msl_conn_charp),

	_C6EV("send roomlist req", c6_req_roomlist, c6msl_conn),
	_C6EV("send roominfo req", c6_req_roominfo, c6msl_conn_charp),
	_C6EV("send room enter", c6_room_enter, c6msl_conn_charp),
	_C6EV("send room leave", c6_room_leave, c6msl_conn_charp),
	_C6EV("send room message", c6_send_roommsg, c6msl_conn_charp_charp),
	_C6EV("send room create", c6_create_room, c6msl_conn_charp_char_charp),

	_C6EV("get search values", c6_get_uinfo_elms, 
	      c6msl_conn_charp_uintp_charppp),
	_C6EV("reset search value", c6_reset_uinfo, c6msl_conn),
	_C6EV("set search value", c6_set_uinfo_val, c6msl_conn_charpp_charpp),
	_C6EV("get set search values", c6_get_set_uinfo_val, 
	      c6msl_conn_uintp_charppp_charppp),
	_C6EV("send search by email", c6_ufind_by_email, c6msl_conn_charp),
	_C6EV("send search users", c6_search_nicks, c6msl_conn),
	
	_C6EV(NULL, NULL, NULL)
};
/* *INDENT-ON* */


void
c6_init()
{
	unsigned int i;

	for (i = 0; c6_ev[i].name != NULL; i++) {
		c6_ev[i].id = sig_register(ev_root, c6_ev[i].name,
					   c6_ev[i].marshal);

		if (c6_ev[i].id == 0)
			FATAL("error in sig_register");

		if (!sig_connect_opt_id(ev_root, c6_ev[i].id, SIGNALS_FIRST,
					SIGNALS_FOREVER, c6_ev[i].cb))
			FATAL("error in sig_connect");
	}
}

void
c6_deinit()
{
#if 0
	unsigned int i;

	for (i = 0; c6_ev[i].name != NULL)
		sig_unregister_id(ev_root, c6_ev[i].id);
#endif
}

void
c6_connection_new(connection_t *c)
{
	c->proto_mod = a_new0(c->area, c6_t);

	C6(c)->fd = -1;
	C6(c)->clt_id = 1;
	C6(c)->srv_id = 1;

	SIMPLEQ_INIT(&C6(c)->proom);
	c6_reset_uinfo(c);
}

void
c6_connection_free(connection_t *c)
{
	event_del(&C6(c)->ev);

	if (C6(c)->fd != -1) {
		close(C6(c)->fd);
		C6(c)->fd = -1;
	}

	a_free(c->area, c->proto_mod);
}

static void
parse_indata(connection_t *c, size_t avdata)
{
	struct c6_pkt_head *sv;
	size_t pklen;
	char *ptr;

	ptr = C6(c)->buf;
	avdata += C6(c)->old_len;

	do {
		sv = (struct c6_pkt_head *)ptr;
		pklen = (size_t)ntohs(sv->len) + sizeof(struct c6_pkt_head);

		if (sv->type != 0x20) {
			EMIT(c, "gui error", "invalid packet... skipping data");
			PDEBUG("invalid packet... skipping data");
			
			C6(c)->old_len = 0;
			return;
		}
		
		if (pklen > avdata) {
			PDEBUG("packet not complete... waiting for more data");
			break;
		}
	
		c6_process_input(c, ptr, pklen);

		avdata -= pklen;
		ptr += pklen;

	} while (avdata != 0);


	if (avdata > 0) {
		C6(c)->old_len = avdata;
		memcpy(C6(c)->buf, ptr, avdata);
	} else
		C6(c)->old_len = 0;
}

void
c6_read(int fd, short event, void *arg)
{
	connection_t *c = arg;
	ssize_t res;

	irC6_callback_start(c);

	PDEBUG("reading data\nallocated bytes %u", area_size(c->area));

	switch (event) {
	case EV_READ:
		break;
	case EV_TIMEOUT:
	default:
		EMIT(c, "gui quit", "read timeout");
		irC6_close_connection(c);
		goto end;
	}

	res = read(fd, C6(c)->buf + C6(c)->old_len,
		   sizeof(C6(c)->buf) - C6(c)->old_len);

	switch (res) {
	case -1:
		if (errno == EAGAIN)
			goto end;

		EMIT(c, "gui fatal", "errore in lettura");
	case 0:
		irC6_close_connection(c);
		goto end;
	default:
		parse_indata(c, (size_t)res);
	}

      end:
	irC6_callback_end(c);
}

struct module c6_struct = {
	c6_init,
	c6_deinit,
	c6_connection_new,
	c6_connection_free
};
