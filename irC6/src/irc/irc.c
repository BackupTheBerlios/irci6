
/* $Id: irc.c,v 1.1 2005/04/16 09:23:25 benkj Exp $ */

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
#include "error.h"
#include "socket.h"
#include "str_missing.h"

void irc_read(int, short, void *);
static struct event listen_event;
static int listen_fd;

static void
add_connection(int fd, short event, void *arg)
{
	connection_t *c;
	area_t *area;
	struct timeval tv;


	switch (event) {
	case EV_READ:
		break;
	case EV_TIMEOUT:
	default:
		event_del(&listen_event);
		return;
	}

	PDEBUG("new connection");

	c = irC6_new_connection();

	IRC(c)->fd = local_accept(fd);

	IRC(c)->fout = fdopen(IRC(c)->fd, "w");
#ifdef __GNU_LIBRARY__
	if (IRC(c)->fout == NULL)
		EFATAL("can't open file descriptor");

	setlinebuf(IRC(c)->fout);
#else
	if (IRC(c)->fout == NULL || setlinebuf(IRC(c)->fout) == -1)
		EFATAL("can't open file descriptor");
#endif

	if (!get_my_ip(IRC(c)->fd, &IRC(c)->ip))
		EMIT(c, "gui fatal",
		     "impossibile ottenere il proprio indirizzo");

	tv.tv_sec = 20;
	tv.tv_usec = 0;

	event_set(&IRC(c)->ev, IRC(c)->fd, EV_READ | EV_PERSIST, irc_read, c);
	event_add(&IRC(c)->ev, &tv);
}

#define _IRCEV(name, cb, msl)						\
	{ name, (marshal_callback_t)cb, msl, 0}


/* *INDENT-OFF* */
static struct {
	char *name;
	marshal_callback_t cb;
	marshal_func_t marshal;
	unsigned int id;
} irc_ev[] = {
	_IRCEV("gui fatal", irc_out_fatal, ircmsl_conn_charp),
	_IRCEV("gui warning", irc_out_warning, ircmsl_conn_charp),
	_IRCEV("gui quit", irc_out_quit, ircmsl_conn_charp),
	_IRCEV("gui welcome", irc_out_welcome, ircmsl_conn_charp),
	_IRCEV("gui invalid user", irc_out_invuser, ircmsl_conn),
	_IRCEV("gui invalid pass", irc_out_invpass, ircmsl_conn),
	_IRCEV("gui nick already connected", irc_out_alrconn, ircmsl_conn),
	_IRCEV("gui login done", irc_out_logindone, ircmsl_conn_saddr),
	_IRCEV("gui banners", irc_out_banners, ircmsl_conn_uint_charpp),
	_IRCEV("gui bottons", irc_out_banners, ircmsl_conn_uint_charpp),
	_IRCEV("gui message", irc_out_message, 
	       ircmsl_conn_charp_charp_charp_charp),
	_IRCEV("gui packet error", irc_out_pkerror, ircmsl_conn_uint),
	_IRCEV("gui got users", irc_out_users, ircmsl_conn_uint_charpp),
	_IRCEV("gui user online", irc_out_user_online, ircmsl_conn_charp),
	_IRCEV("gui user offline", irc_out_user_offline, ircmsl_conn_charp),
	_IRCEV("gui user change status", irc_out_user_chg_status, 
	       ircmsl_conn_charp_uint),
	_IRCEV("gui user info", NULL,
	       ircmsl_conn_charp_charp_timep_addrp_uint_charpp),
	_IRCEV("gui room list", irc_out_roomlist, 
	       ircmsl_conn_uint_charpp_uintp_uintp),
	_IRCEV("gui room info", NULL, 
	       ircmsl_conn_charp_charp_charp_uint_uint),
	_IRCEV("gui room user list", irc_out_room_ulist, 
	       ircmsl_conn_charp_uint_charpp),
	_IRCEV("gui room message", irc_out_room_message, 
	       ircmsl_conn_charp_charp_charp_charp_charp),
	_IRCEV("gui room user enter", irc_out_room_uenter, 
	       ircmsl_conn_charp_charp),
	_IRCEV("gui room user leave", irc_out_room_uleave, 
	       ircmsl_conn_charp_charp),
	_IRCEV("gui room can leave", irc_out_room_canleave, ircmsl_conn_charp),
	_IRCEV("gui room notexist", irc_out_room_notexist, ircmsl_conn_charp),
	_IRCEV("gui room is full", irc_out_room_full, ircmsl_conn_charp),
	_IRCEV("gui room already exist", irc_out_room_alrexist, 
	       ircmsl_conn_charp),
	_IRCEV("gui found users", irc_out_foundusers, ircmsl_conn_uint_charpp),
	_IRCEV("gui select nick", irc_out_selnick, 
	       ircmsl_conn_uint_charpp_charpp),

#ifdef IRC6_DEBUG
	_IRCEV("gui dump data", irc_dump_data, ircmsl_conn_ptr_size),
#endif

	_IRCEV(NULL, NULL, NULL)
};

/* *INDENT-ON* */

void
irc_init()
{
	unsigned int i;

	for (i = 0; irc_ev[i].name != NULL; i++) {
		irc_ev[i].id = sig_register(ev_root, irc_ev[i].name,
					    irc_ev[i].marshal);

		if (irc_ev[i].id == 0)
			FATAL("error in sig_register");

		if (irc_ev[i].cb == NULL)
			continue;

		if (!sig_connect_opt_id(ev_root, irc_ev[i].id, SIGNALS_FIRST,
					SIGNALS_FOREVER, irc_ev[i].cb))
			FATAL("error in sig_connect");
	}

	PDEBUG("listening");

	listen_fd = (use_inetd) ? STDIN_FILENO :
	    local_listen(listen_host, listen_port, max_conn);

	event_set(&listen_event, listen_fd, EV_READ | EV_PERSIST,
		  add_connection, NULL);

	event_add(&listen_event, NULL);

}

void
irc_deinit()
{
	close(listen_fd);
}


void
irc_connection_new(connection_t *c)
{
	PDEBUG("irc_conn new");

	strlcpy(c->nick, "user", sizeof(c->nick));

	c->gui_mod = a_new0(c->area, irc_t);

	SIMPLEQ_INIT(&IRC(c)->cmd_list);
}

void
irc_connection_free(connection_t *c)
{
	event_del(&IRC(c)->ev);

	if (IRC(c)->fd != -1) {
		close(IRC(c)->fd);
		IRC(c)->fd = -1;
	}

	a_free(c->area, c->gui_mod);
}

void
irc_read(int fd, short event, void *arg)
{
	connection_t *c = arg;
	ssize_t res, sz, offs;

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

	res = read(fd, IRC(c)->buf + IRC(c)->old_len,
		   sizeof(IRC(c)->buf) - IRC(c)->old_len);

	switch (res) {
	case -1:
		if (errno == EAGAIN)
			goto end;

		EMIT(c, "gui fatal", "errore in lettura");
	case 0:
		irC6_close_connection(c);
		goto end;
	default:
		sz = IRC(c)->old_len;
		offs = 0;
		res += IRC(c)->old_len;
	}

	while (sz < res)
		switch (IRC(c)->buf[sz + offs]) {
		case '\n':
		case '\0':
			if (sz > 0 && IRC(c)->buf[sz + offs - 1] == '\r')
				irc_process_input(c, IRC(c)->buf + offs,
						  sz - 1);
			else
				irc_process_input(c, IRC(c)->buf + offs, sz);

			offs += sz + 1;
			res -= sz + 1;
			sz = 0;
			break;
		default:
			sz++;
		}

	if (res > 0) {
		IRC(c)->old_len = res;
		memcpy(IRC(c)->buf, IRC(c)->buf + offs, (size_t)res);
	} else
		IRC(c)->old_len = 0;

      end:
	irC6_callback_end(c);
}

struct module irc_struct = {
	irc_init,
	irc_deinit,
	irc_connection_new,
	irc_connection_free
};
