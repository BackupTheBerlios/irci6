
/* $Id: signals.c,v 1.1 2005/04/16 09:23:29 benkj Exp $ */

/*
 * Copyright (c) 2004
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/tree.h>
#include <sys/queue.h>
#include <errno.h>

#include <hash.h>
#include <signals.h>

SLIST_HEAD(siginst_head, signals_instance);
TAILQ_HEAD(handlers_head, handler);

struct signals_instance
{
	struct siginst_head nodes;
	struct signals_instance *parent;
	
	area_t *area;
	hashtable_t *htable;

	unsigned int emitting;
	unsigned int quit:1;
	unsigned int destroyed:1;

	SLIST_ENTRY(signals_instance) entry;
};

struct signals
{
	unsigned int id;

	unsigned int flags;

	marshal_func_t marshal;

	struct handlers_head handlers;
};

struct handler {
	unsigned int priority;
	unsigned int count;

	marshal_callback_t func;

	TAILQ_ENTRY(handler) entry;
};

#define HANDLER_FOREACH(_h, _s, _hnxt) \
	for (_h = TAILQ_FIRST(&(_s)->handlers); _h != NULL; 		\
	    _h = _hnxt) 
#define HANDLER_UPDATE_NEXT(_h, _hnxt)					\
	_hnxt = TAILQ_NEXT(_h, entry)

siginst_t *
sig_instance_new(area_t *area)
{
	siginst_t *si;
	area_t *anode;

	if ((anode = area_node(area)) == NULL)
		return NULL;
	
	if ((si = a_new0(anode, siginst_t)) == NULL)
		return NULL;

	SLIST_INIT(&si->nodes);
	si->area = anode;
	if ((si->htable = hashtable_new(anode, HASHTABLE_DUPDATA)) == NULL)
		goto error;

	return si;
error:
	area_destroy(anode);
	return NULL;
}

siginst_t *
sig_instance_node(area_t *area, siginst_t *parent)
{
	siginst_t *si;
	area_t *anode;

	if ((anode = area_node(area ? area : parent->area)) == NULL)
		return NULL;

	if ((si = a_new0(anode, siginst_t)) == NULL)
		return NULL;

	SLIST_INIT(&si->nodes);
	si->area = anode;
	si->parent = parent;

	if ((si->htable = hashtable_new(anode, HASHTABLE_DUPDATA)) == NULL)
		goto error;

	return si;
error:
	area_destroy(anode);
	return NULL;
}

void
sig_instance_quit(siginst_t *cur)
{
	if (cur->emitting)
		cur->quit = 1;
}

void
sig_instance_destroy(siginst_t *cur)
{
	siginst_t *s, *snext;

	if (cur->emitting) {
		cur->quit = 1;
		cur->destroyed = 1;

		return;
	}
	
	for (s = SLIST_FIRST(&cur->nodes); s != SLIST_END(&cur->nodes);
	    s = snext) {
 		snext = SLIST_NEXT(s, entry);
		sig_instance_destroy(s);
	}

	if (cur->parent != NULL)
		SLIST_REMOVE(&cur->parent->nodes, cur, signals_instance, entry);

	area_destroy(cur->area);
}

unsigned int
sig_instance_emitting(siginst_t *cur)
{
	return cur->emitting;
}

static struct signals *
find_signal(siginst_t *si, const char *sig)
{
	struct signals *s = NULL;
	hash_t *h = NULL;
	int ret;
	void *data = (void *)sig;

	ret = hashtable_find_h(si->htable, data, strlen(sig) + 1, &h);

	if (ret == HASH_OK)
		hash_getarg(h, &s);

	hash_destroy(si->htable, h);

	return s;
}

static struct signals *
find_signal_id(siginst_t *si, unsigned int id, char **name)
{
	struct signals *s = NULL;
	hash_collision_t *hlist = NULL;
	int elms;

	elms = hashtable_find_id(si->htable, id, &hlist);

	if (elms == 1) {
		s = hlist[0].arg;

		if (name)
			*name = hlist[0].data;
	}

	hash_collisions_destroy(si->htable, hlist);

	return s;
}

static int
add_signal(siginst_t *si, const char *sig, struct signals *s)
{
	hash_t *h = NULL;
	void *data = (void *)sig;
	int ret;

	ret = hashtable_add_h(si->htable, data, strlen(sig) + 1, s, &h);

	if (ret == HASH_OK)
		hash_get(h, &s->id, NULL, NULL, NULL);

	hash_destroy(si->htable, h);

	return (ret == HASH_OK);
}

static int
del_signal(siginst_t *si, const char *sig)
{
	void *data = (void *)sig;

	return (hashtable_del_d(si->htable, data, strlen(sig) + 1) == HASH_OK);
}

unsigned int
sig_get_id(siginst_t *si, const char *sig)
{
	struct signals *s;

	return ((s = find_signal(si, sig)) == NULL) ? 0 : s->id;
}

unsigned int
sig_register(siginst_t *si, const char *sig, marshal_func_t mfunc)
{
	struct signals *s;

	if (find_signal(si, sig))
		return 0;

	if ((s = a_new0(si->area, struct signals)) == NULL)
		return 0;

	if (!add_signal(si, sig, s)) {
		a_free(si->area, s);
		return 0;
	}

	s->marshal = mfunc;
	TAILQ_INIT(&s->handlers);

	return s->id;
}

int
sig_unregister(siginst_t *si, const char *sig)
{
	return del_signal(si, sig);
}

static void 
insert_with_priority(struct signals *s, struct handler *add)
{
	struct handler *tmp;
	struct handlers_head *head = &s->handlers;

	switch (add->priority) {
	case SIGNALS_LAST:
		TAILQ_INSERT_TAIL(head, add, entry);
		break;
	case SIGNALS_FIRST:
		TAILQ_INSERT_HEAD(head, add, entry);
		break;
	default:
		TAILQ_FOREACH(tmp, head, entry) {
			if (add->priority > tmp->priority)
				TAILQ_INSERT_BEFORE(tmp, add, entry);
		}
	}
}

static unsigned int
connect_signal(siginst_t *si, struct signals *s, 
    unsigned int priority, unsigned int count, marshal_callback_t cb)
{
	struct handler *hdl;
	
	if ((hdl = a_new0(si->area, struct handler)) == NULL)
		return 0;

	hdl->priority = priority;
	hdl->count = count;
	hdl->func = cb;

	insert_with_priority(s, hdl);

	return 1;
}
unsigned int
_sig_connect(siginst_t *si, const char *sig, 
	unsigned int priority, unsigned int count, marshal_callback_t cb)
{
	struct signals *s;

	if ((s = find_signal(si, sig)) == NULL) {
		struct signals *os = NULL;
		siginst_t *tmp;

		for (tmp = si->parent; 
		     tmp != NULL && os == NULL; 
		     tmp = tmp->parent)
			os = find_signal(tmp, sig);

		if (os == NULL)
			return 0;
		if ((s = a_new0(si->area, struct signals)) == NULL)
			return 0;

		if (!add_signal(si, sig, s)) {
			a_free(si->area, s);
			return 0;
		}

		s->marshal = os->marshal;
		s->flags = os->flags;
		TAILQ_INIT(&s->handlers);
	}

	return connect_signal(si, s, priority, count, cb);
}

unsigned int
_sig_connect_id(siginst_t *si, unsigned int id, 
	unsigned int priority, unsigned int count, marshal_callback_t cb)
{
	struct signals *s;

	if ((s = find_signal_id(si, id, NULL)) == NULL) {
		struct signals *os = NULL;
		siginst_t *tmp;
		char *name = NULL;

		for (tmp = si->parent; 
		     tmp != NULL && os == NULL; 
		     tmp = tmp->parent)
			os = find_signal_id(tmp, id, &name);

		if (os == NULL)
			return 0;
		if ((s = a_new0(si->area, struct signals)) == NULL)
			return 0;

		if (!add_signal(si, name, s)) {
			a_free(si->area, s);
			return 0;
		}

		s->marshal = os->marshal;
		s->flags = os->flags;
		TAILQ_INIT(&s->handlers);
	}

	return connect_signal(si, s, priority, count, cb);
}

static void
del_handler(siginst_t *si, struct signals *s, struct handler *hdl)
{
	TAILQ_REMOVE(&s->handlers, hdl, entry);
	a_free(si->area, hdl);
}

static unsigned int
disconnect_signal(siginst_t *si, struct signals *s, marshal_callback_t cb)
{
	struct handler *hdl, *hdl_nxt;

	HANDLER_FOREACH(hdl, s, hdl_nxt) {
		HANDLER_UPDATE_NEXT(hdl, hdl_nxt);
		
		if (hdl->func == cb) {
			del_handler(si, s, hdl);
			return 1;
		}
	}
	
	return 0;
}

unsigned int
_sig_disconnect(siginst_t *si, const char *sig, 
	marshal_callback_t cb)
{
	struct signals *s;

	return ((s = find_signal(si, sig)) != NULL) ? 
	    disconnect_signal(si, s, cb) : 0;
}

unsigned int
_sig_disconnect_id(siginst_t *si, const char *sig, 
	marshal_callback_t cb)
{
	struct signals *s;

	return ((s = find_signal(si, sig)) != NULL) ? 
	    disconnect_signal(si, s, cb) : 0;
}

unsigned int
sig_set_flags(siginst_t *si, const char *sig, unsigned int flags)
{
	struct signals *s;
	siginst_t *tmp;

	SLIST_FOREACH(tmp, &si->nodes, entry) {
		if (!sig_set_flags(tmp, sig, flags))
			return 0;
	}
	
	if((s = find_signal(si, sig)) != NULL) 
		s->flags |= flags;

	return (s != NULL);
}

unsigned int
sig_set_flags_id(siginst_t *si, unsigned int id, unsigned int flags)
{
	struct signals *s;
	siginst_t *tmp;

	SLIST_FOREACH(tmp, &si->nodes, entry) {
		if (!sig_set_flags_id(tmp, id, flags))
			return 0;
	}
	
	if((s = find_signal_id(si, id, NULL)) != NULL) 
		s->flags |= flags;

	return (s != NULL);
}

unsigned int
sig_unset_flags(siginst_t *si, const char *sig, unsigned int flags)
{
	struct signals *s;
	siginst_t *tmp;

	SLIST_FOREACH(tmp, &si->nodes, entry) {
		if (!sig_unset_flags(tmp, sig, flags))
			return 0;
	}
	
	if((s = find_signal(si, sig)) != NULL) 
		s->flags &= ~flags;

	return (s != NULL);
}

unsigned int
sig_unset_flags_id(siginst_t *si, unsigned int id, unsigned int flags)
{
	struct signals *s;
	siginst_t *tmp;

	SLIST_FOREACH(tmp, &si->nodes, entry) {
		if (!sig_unset_flags_id(tmp, id, flags))
			return 0;
	}
	
	if((s = find_signal_id(si, id, NULL)) != NULL) 
		s->flags &= ~flags;

	return (s != NULL);
}

unsigned int
sig_isset_flags(siginst_t *si, const char *sig, unsigned int flags)
{
	struct signals *s;

	return ((s = find_signal(si, sig)) != NULL) ? 
	    ((s->flags & flags) == flags)  : 0;
}

unsigned int
sig_isset_flags_id(siginst_t *si, unsigned int id, unsigned int flags)
{
	struct signals *s;

	return ((s = find_signal_id(si, id, NULL)) != NULL) ? 
	    ((s->flags & flags) == flags)  : 0;
}

static void
sig_emit_real(siginst_t *si, struct signals *s, va_list ap)
{
	struct handler *hdl, *hdl_nxt;

	si->emitting++;

	HANDLER_FOREACH(hdl, s, hdl_nxt) {
		s->marshal(hdl->func, ap, NULL);

		HANDLER_UPDATE_NEXT(hdl, hdl_nxt);

		if (hdl->count != SIGNALS_FOREVER && --hdl->count == 0)
			del_handler(si, s, hdl);

		if (s->flags & SIGNALS_LOCK || si->quit)
			break;
	}
	
	si->emitting--;

	if (!si->emitting) {
		if (si->destroyed)
			sig_instance_destroy(si);

		si->quit = 0;
	}
}

void
sig_emit(siginst_t *si, const char *sig, ...)
{
	struct signals *s = NULL;
	siginst_t *tmp, *prev;
	va_list ap;

	va_start(ap, sig);

	for (tmp = si; tmp != NULL; si = tmp, tmp = tmp->parent) {
		s = find_signal(tmp, sig);

		if (s == NULL || s->flags & SIGNALS_LOCK)
			continue;

		sig_emit_real(si, s, ap);
	}

	va_end(ap);
}

void
sig_vemit(siginst_t *si, const char *sig, va_list ap)
{
	struct signals *s = NULL;
	siginst_t *tmp;

	for (tmp = si; tmp != NULL; si = tmp, tmp = tmp->parent) {
		s = find_signal(tmp, sig);

		if (s == NULL || s->flags & SIGNALS_LOCK)
			continue;

		sig_emit_real(si, s, ap);
	}

}

void
sig_emit_id(siginst_t *si, unsigned int id, ...)
{
	struct signals *s = NULL;
	siginst_t *tmp;
	va_list ap;

	va_start(ap, id);

	for (tmp = si; tmp != NULL; si = tmp, tmp = tmp->parent) {
		s = find_signal_id(tmp, id, NULL);

		if (s == NULL || s->flags & SIGNALS_LOCK)
			continue;

		sig_emit_real(si, s, ap);
	}

	va_end(ap);
}

void
sig_vemit_id(siginst_t *si, unsigned int id, va_list ap)
{
	struct signals *s = NULL;
	siginst_t *tmp;

	for (tmp = si; tmp != NULL; si = tmp, tmp = tmp->parent) {
		s = find_signal_id(tmp, id, NULL);

		if (s == NULL || s->flags & SIGNALS_LOCK)
			continue;

		sig_emit_real(si, s, ap);
	}
}


