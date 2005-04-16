
/* $Id: list.h,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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

struct chanlist {
	char *name;
	char *cname;

	hashtable_t *nicks;
};

#define CHANLIST_FOREACH(list, _hh,  clstruct) 				\
	_hh = hash_newvoid(list);					\
	for (hashtable_begin(list, _hh), hash_getarg(_hh, clstruct);	\
	    !hashtable_end(list, _hh) || (_hash_destroy(list, _hh), 0);	\
	    hashtable_next(list, _hh), hash_getarg(_hh, clstruct))

#define CHANLIST_FOREACH_NICK(clstruct, _hh,  nick)	 		\
	_hh = hash_newvoid((clstruct)->nicks);				\
	for (hashtable_begin((clstruct)->nicks, _hh), 			\
	    hash_get(_hh, NULL, (void **)nick, NULL, NULL);		\
	    !hashtable_end((clstruct)->nicks, _hh) || 			\
	    (_hash_destroy((clstruct)->nicks, _hh), 0);			\
	    hashtable_next((clstruct)->nicks, _hh),			\
	    hash_get(_hh, NULL, (void **)nick, NULL, NULL))


struct nicklist {
	char *name;

	uint8_t status;
	uint8_t black_list:1;
};

#define NICKLIST_FOREACH(list, _hh,  nlstruct) 				\
	_hh = hash_newvoid(list);					\
	for (hashtable_begin(list, _hh), hash_getarg(_hh, nlstruct);	\
	    !hashtable_end(list, _hh) || (_hash_destroy(list, _hh), 0);	\
	    hashtable_next(list, _hh), hash_getarg(_hh, nlstruct))


/* list.c */
int chanlist_add(connection_t *, hashtable_t *, char *, char *);
int chanlist_del(connection_t *, hashtable_t *, char *);
int chanlist_getstruct(hashtable_t *, char *, struct chanlist **);
int chanlist_addnick(struct chanlist *, char *);
int chanlist_delnick(struct chanlist *, char *);
int chanlist_getnick(struct chanlist *, char *);
int nicklist_add(connection_t *, hashtable_t *, char *);
int nicklist_del(connection_t *, hashtable_t *, char *);
int nicklist_getstruct(hashtable_t *, char *, struct nicklist **);
