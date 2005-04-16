
/* $Id: socket.c,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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
#include "socket.h"
#include "error.h"

int
resolv(host, serv, sin)
    char *host, *serv;
    struct sockaddr_in *sin;
{
	struct hostent *hp;
	struct servent *se;
	unsigned long p;
	char *err;

	memset(sin, 0, sizeof(struct sockaddr_in));

	sin->sin_family = AF_INET;

	if (host != NULL)
		if (!inet_aton(host, &sin->sin_addr)) {
			hp = gethostbyname(host);

			if (hp != NULL &&
			    (size_t)hp->h_length <= sizeof(sin->sin_addr))
				memcpy(&sin->sin_addr.s_addr, hp->h_addr,
				       hp->h_length);
			else
				return 0;
		}

	if (serv != NULL) {
		err = NULL;
		se = getservbyname(serv, "tcp");
		p = strtoul(serv, &err, 10);

		if (se != NULL && (se->s_port & ~0xffff) == 0)
			sin->sin_port = (uint16_t)se->s_port;
		else if ((err == NULL || *err == '\0') && (p & ~0xffff) == 0)
			sin->sin_port = htons((uint16_t)p);
		else
			return 0;
	}

	return 1;
}

static int
set_nonblock(fd)
    int fd;
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags != -1)
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1)
			return 1;

	return 0;
}

int
local_listen(host, port, backlog)
    char *host, *port;
    int backlog;
{
	struct sockaddr_in sin;
	struct linger li;
	int sd, on;

	resolv(host, port, &sin);

	if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		EFATAL("can't make socket");

	li.l_onoff = 1;
	li.l_linger = 0;
	on = 1;

	setsockopt(sd, SOL_SOCKET, SO_LINGER, &li, sizeof(li));
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1 ||
	    listen(sd, backlog) == -1)
		EFATAL("can't bind %s:%s", host, port);

	return sd;
}

int
local_accept(fd)
    int fd;
{
	int sd;

	if ((sd = accept(fd, NULL, NULL)) == -1)
		EFATAL("error accepting connection");

	if (!set_nonblock(sd))
		EFATAL("error accepting connection");

	return sd;
}

int
connect_addr(sin)
    struct sockaddr_in *sin;
{
	int sd;

	if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return -1;

	if (connect(sd, (struct sockaddr *)sin, sizeof(*sin)) == -1)
		return -1;

	set_nonblock(sd);

	return sd;
}

int
get_my_ip(sd, in)
    int sd;
    struct in_addr *in;
{
	struct sockaddr_in sin;
	socklen_t len = sizeof(struct sockaddr_in);
	int ret;

	ret = getsockname(sd, (struct sockaddr *)&sin, &len);

	if (ret == -1 || len != sizeof(struct sockaddr_in))
		return 0;

	memcpy(in, &sin.sin_addr, sizeof(struct in_addr));

	return 1;
}

ssize_t
atomicio(func, fd, ptr, size)
    atomfunc_t func;
    int fd;
    void *ptr;
    size_t size;
{
	ssize_t res;
	size_t pos;

	for (pos = 0; size > pos;) {
		res = (func) (fd, (char *)ptr + pos, size - pos);

		switch (res) {
		case -1:
			if (errno == EINTR || errno == EAGAIN)
				continue;
		case 0:
			return 0;
		default:
			pos += (size_t)res;
		}
	}

	return 1;
}
