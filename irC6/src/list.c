
/* $Id: list.c,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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
#include "error.h"
#include "str_missing.h"
#include "list.h"

int
chanlist_add(connection_t *c, hashtable_t *list, char *room, char *croom)
{
	struct chanlist *cl;
	int ret;

	cl = a_new0(c->area, struct chanlist);

	cl->name = a_strdup(c->area, room);

	ret = hashtable_add_d(list, cl->name, strlen(cl->name), cl);
	if (ret <= 0) {
		a_free(c->area, cl->name);
		a_free(c->area, cl);

		return (ret == HASH_DUP) ? 0 : 1;
	}

	cl->cname = a_strdup(c->area, croom);
	cl->nicks = hashtable_new(c->area, (2 & HASHTABLE_CACHEMASK) |
				  HASHTABLE_CHMATCH | HASHTABLE_DUPDATA);

	return 1;
}

int
chanlist_del(connection_t *c, hashtable_t *list, char *room)
{
	hash_t *hh = NULL;
	struct chanlist *cl;
	int ret;

	ret = hashtable_del_h(list, room, strlen(room), &hh);

	if (ret == HASH_OK) {
		hash_getarg(hh, &cl);

		hashtable_destroy(cl->nicks);
		a_free(c->area, cl->name);
		a_free(c->area, cl->cname);
		a_free(c->area, cl);
	}

	hash_destroy(list, hh);

	return (ret == HASH_OK);
}

int
chanlist_getstruct(hashtable_t *list, char *room, struct chanlist **clstruct)
{
	hash_t *hh = NULL;
	int ret;

	ret = hashtable_find_h(list, room, strlen(room), &hh);

	if (ret == HASH_OK)
		hash_getarg(hh, clstruct);
	hash_destroy(list, hh);

	return (ret == HASH_OK);
}

int
chanlist_addnick(struct chanlist *cl, char *nick)
{
	int ret;

	ret = hashtable_add_d(cl->nicks, nick, strlen(nick) + 1, NULL);

	if (ret > 0)
		return 1;
	else if (ret < 0)
		return -1;
	else
		return 0;
}

int
chanlist_delnick(struct chanlist *cl, char *nick)
{
	return (hashtable_del_d(cl->nicks, nick, strlen(nick) + 1) == HASH_OK);
}

int
chanlist_getnick(struct chanlist *cl, char *nick)
{
	return (hashtable_find_d(cl->nicks, nick, strlen(nick) + 1) == HASH_OK);
}


int
nicklist_add(connection_t *c, hashtable_t *list, char *nick)
{
	struct nicklist *nl;
	int ret;

	nl = a_new0(c->area, struct nicklist);

	nl->name = a_strdup(c->area, nick);

	ret = hashtable_add_d(list, nl->name, strlen(nl->name), nl);

	if (ret <= 0) {
		a_free(c->area, nl->name);
		a_free(c->area, nl);

		return (ret == HASH_DUP) ? 0 : -1;
	}

	return 1;
}

int
nicklist_del(connection_t *c, hashtable_t *list, char *nick)
{
	struct nicklist *nl;
	hash_t *hh = NULL;
	int ret;

	ret = hashtable_del_h(list, nick, strlen(nick), &hh);

	if (ret == HASH_OK) {
		hash_getarg(hh, &nl);

		a_free(c->area, nl->name);
		a_free(c->area, nl);
	}

	hash_destroy(list, hh);

	return (ret == HASH_OK);
}

int
nicklist_getstruct(hashtable_t *list, char *nick, struct nicklist **nlstruct)
{
	hash_t *hh = NULL;
	int ret;

	ret = hashtable_find_h(list, nick, strlen(nick), &hh);

	if (ret == HASH_OK)
		hash_getarg(hh, nlstruct);
	hash_destroy(list, hh);

	return (ret == HASH_OK);
}
