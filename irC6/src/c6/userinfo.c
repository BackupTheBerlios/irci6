
/* $Id: userinfo.c,v 1.1 2005/04/16 09:23:23 benkj Exp $ */

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
#include "str_missing.h"


static int
_getid(char **name, int *id)
{
	struct searchui *sui;
	char *err = NULL;
	size_t i;

	if (*name == NULL && (*id < 0 || *id >= (int)c6_userinfo_elms))
		return 0;

	if (*name == NULL) {
		*name = (char *)c6_userinfo[*id].name;

		return 1;
	} else {
		*id = (int)strtoul(*name, &err, 10);

		if (err && *err != '\0') {
			sui = c6_userinfo_hash(*name, strlen(*name));

			if (sui == NULL)
				return 0;

			for (i = 0; i < sui->elms; i++) {
				if (sui->v[i].val != -1)
					continue;

				*id = sui->v[i].id;

				if (strcasecmp(*name, c6_userinfo[*id].name))
					continue;

				return 1;
			}
		} else if (*id >= 0 && *id < (int)c6_userinfo_elms) {
			*name = (char *)c6_userinfo[*id].name;

			return 1;
		}
	}

	return 0;
}

static int
_getval(char **name, int id, int *val)
{
	struct searchui *sui;
	char *err = NULL;
	size_t i;

	if (id < 0 || id >= (int)c6_userinfo_elms)
		return 0;

	if (*name == NULL) {
		if (*val >= (int)c6_userinfo[id].entries || *val < 0)
			return 0;

		*name = (char *)c6_userinfo[id].info[*val];

		return 1;
	} else {
		*val = (int)strtoul(*name, &err, 10);

		if (err && *err != '\0') {
			sui = c6_userinfo_hash(*name, strlen(*name));

			if (sui == NULL)
				return 0;

			for (i = 0; i < sui->elms; i++) {
				if (sui->v[i].id != id)
					continue;

				*val = sui->v[i].val;

				if (strcasecmp(*name, 
					       c6_userinfo[id].info[*val]))
					continue;

				return 1;
			}
		} else if (*val >= 0 && *val < (int)c6_userinfo[id].entries) {
			*name = (char *)c6_userinfo[id].info[*val];

			return 1;
		}
	}

	return 0;
}

void
c6_get_uinfo_elms(connection_t *c, char *name, unsigned int *elms, char ***vec)
{
	struct searchui *sui;
	unsigned int i;
	int id;

	if (name == NULL) {
		*vec = a_nnew(c->atmp, c6_userinfo_elms, char *);

		*elms = c6_userinfo_elms;

		for (i = 0; i < c6_userinfo_elms; i++)
			(*vec)[i] = (char *)c6_userinfo[i].name;


		return;
	}

	if (_getid(&name, &id)) {
		*vec = a_nnew(c->atmp, c6_userinfo[id].entries, char *);

		*elms = c6_userinfo[id].entries;

		for (i = 0; i < c6_userinfo[id].entries; i++)
			(*vec)[i] = (char *)c6_userinfo[id].info[i];

		return;
	}

      error:
	*elms = 0;
}

void
c6_reset_uinfo(connection_t *c)
{
	memset(C6(c)->search_id, 0xff, sizeof(C6(c)->search_id));
}

void
c6_set_uinfo_val(connection_t *c, char **name, char **value)
{
	int id, val;

	if (!_getid(name, &id)) {
		*name = NULL;
		return;
	}

	if (id < 0 || id >= (int)c6_userinfo_elms)
		FATAL("error in userinfo hash");

	if (!_getval(value, id, &val)) {
		*value = NULL;
		return;
	}

	if (val < 0 || val >= (int)c6_userinfo[id].entries)
		FATAL("error in userinfo hash");

	C6(c)->search_id[id] = val;
}

void
c6_get_set_uinfo_val(connection_t *c, unsigned int *elms,
		     char ***name, char ***value)
{
	unsigned int i, k, tot;

	for (i = 0, tot = 0; i < 256 && i < c6_userinfo_elms; i++) {
		if (C6(c)->search_id[i] == 0xff)
			continue;

		tot++;
	}

	*name = a_nnew(c->atmp, tot, char *);
	*value = a_nnew(c->atmp, tot, char *);
	*elms = tot;

	for (i = 0, k = 0; i < 256 && i < c6_userinfo_elms; i++) {
		if (C6(c)->search_id[i] == 0xff)
			continue;

		if (C6(c)->search_id[i] >= (int)c6_userinfo[i].entries)
			FATAL("error in userinfo hash");

		(*name)[k] = (char *)c6_userinfo[i].name;
		(*value)[k] = (char *)c6_userinfo[i].info[C6(c)->search_id[i]];

		k++;
	}
}
