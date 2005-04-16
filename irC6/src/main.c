
/* $Id: main.c,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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

#include "defs.h"
#include "error.h"
#include "irC6.h"
#include "socket.h"
#include "str_missing.h"
#include "version.h"

extern struct module c6_struct;
extern struct module irc_struct;

static struct module *proto_mod = &c6_struct;
static struct module *gui_mod = &irc_struct;

area_t *root;
siginst_t *ev_root;
static jmp_buf jmp_zero;

SLIST_HEAD(connection_list_, connection_entry);
static struct connection_list_ connection_list;

int is_daemon;
int can_squit;
int max_conn;
int use_inetd;
time_t srv_time;

char *listen_host = "127.0.0.1", *listen_port = "6667";
char *server_host = "c6login.tin.it", *server_port = "4800";

extern char *__progname;

connection_t *
irC6_new_connection()
{
	connection_t *c;
	area_t *area;

	area = area_node(root);

	c = a_new0(area, connection_t);

	SLIST_INSERT_HEAD(&connection_list, c, list);

	c->area = area;
	c->siginst = sig_instance_node(area, ev_root);
	c->nicks = hashtable_new(area, (0 & HASHTABLE_CACHEMASK) |
				 HASHTABLE_CHMATCH);
	c->chans = hashtable_new(area, (0 & HASHTABLE_CACHEMASK) |
				 HASHTABLE_CHMATCH);

	proto_mod->connection_new(c);
	gui_mod->connection_new(c);

	return c;
}

void
irC6_close_connection(connection_t *c)
{
	jmp_buf jmp;

	memcpy(&jmp, &c->jmp, sizeof(jmp_buf));

	proto_mod->connection_free(c);
	gui_mod->connection_free(c);

	SLIST_REMOVE(&connection_list, c, connection_entry, list);

	/* XXX: controllare meglio possibili errori */
	area_destroy(c->area);

	if (memcmp(jmp, jmp_zero, sizeof(jmp_buf)))
		longjmp(jmp, 1);
}

static void
exit_program()
{
	static int lock;
	connection_t *c;
	sigset_t set;

	if (lock)
		return;
	lock = 1;

	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	PDEBUG("exiting...");

	SLIST_FOREACH(c, &connection_list, list) {
		proto_mod->connection_free(c);
		gui_mod->connection_free(c);
	}

	proto_mod->deinit();
	gui_mod->deinit();

	area_destroy(root);

	_exit(0);
}

static void
memory_error()
{
	EFATAL("memory error");
}

int
main(argc, argv)
    int argc;
    char **argv;
{
	int cc;
	extern char *__progname;

	tzset();
	srv_time = time(NULL);

	SLIST_INIT(&connection_list);
	root = area_root((area_errfunc_t)memory_error, NULL);
	ev_root = sig_instance_new(root);

	is_daemon = 0;
	can_squit = 1;
	max_conn = 1;
	use_inetd = 0;

	signal(SIGINT, exit_program);
	signal(SIGTERM, exit_program);
	signal(SIGHUP, exit_program);
	signal(SIGQUIT, exit_program);
	signal(SIGPIPE, exit_program);
	atexit(exit_program);

	while ((cc = getopt(argc, argv, "l:p:H:P:idm:hv")) != -1)
		switch (cc) {
		case 'l':
			listen_host = a_strdup(root, optarg);
			break;
		case 'p':
			listen_port = a_strdup(root, optarg);
			break;
		case 'H':
			server_host = a_strdup(root, optarg);
			break;
		case 'P':
			server_port = a_strdup(root, optarg);
			break;
		case 'd':
			is_daemon = 1;
			break;
		case 'i':
			use_inetd = 1;
			break;
		case 'm':
			max_conn = atoi(optarg);
			break;
		case 'Q':
			can_squit = 0;
			break;
		case 'v':
			printf("irC6 v%s\n", VERSION);
			exit(EXIT_SUCCESS);
		case 'h':
			printf
			    ("usage: %s\t[-l listen_host] [-p listen_port]\n"
			     "\t\t[-H server_host] [-P server_port]\n"
			     "\t\t[-m max_connections] "
			     "[-i] [-d] [-Q] [-v] [-h]\n", argv[0]);
			exit(EXIT_SUCCESS);
		default:
			printf("use `%s -h' to see help\n", argv[0]);
			exit(EXIT_FAILURE);
		}

	if (is_daemon)
		switch (fork()) {
		case -1:
			EFATAL("can't fork");
		case 0:
			setsid();

			if (fork())
				exit(EXIT_SUCCESS);
			break;
		default:
			exit(EXIT_SUCCESS);
		}

	if (use_inetd)
		is_daemon = 1;

	if (is_daemon) {
		chdir("/tmp");

		openlog(__progname, LOG_PID, LOG_DAEMON);
	}

	event_init();

	proto_mod->init();
	gui_mod->init();

	event_dispatch();

	return 0;
}
