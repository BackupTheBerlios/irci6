
/* $Id: area.c,v 1.1 2005/04/16 09:23:26 benkj Exp $ */

/*
 * Copyright (c) 2002,2003
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/queue.h>

#define _AREA_INTERNAL
#include "area.h"

SLIST_HEAD(A_node_head, A_node);
LIST_HEAD(A_chunk_head, A_chunk);

struct A_err {
	void (*func) (void *);
	void *arg;
};

struct A_node {
	struct A_err *err;
	struct A_node_head nodes;
	struct A_node *parent;
	size_t allocated;

	       SLIST_ENTRY(A_node) node;

	struct A_chunk_head list;
};

struct A_chunk {
	LIST_ENTRY(A_chunk) chunk;
	size_t size;
};

#define _A_ERROR(_ap, _err) do {					\
	errno = _err;							\
	if ((_ap)->err != NULL)						\
		(*(_ap)->err->func)((_ap)->err->arg);			\
} while (0)

#define _A_ERROR_RET(_ap, _err, _ret) do {				\
	errno = _err;							\
	if ((_ap)->err != NULL)						\
		(*(_ap)->err->func)((_ap)->err->arg);			\
	return _ret;							\
} while (0)

#define _A_COND_ERROR(_cond, _ap, _err) 				\
	do { if (_cond) _A_ERROR(_ap, _err); } while (0)

#define _A_COND_ERROR_RET(_cond, _ap, _err, _ret)			\
	do { if (_cond) _A_ERROR_RET(_ap, _err, _ret); } while (0)

#define _A_OFFS(_ptr, _type, _cast) 					\
	(_cast)((char *)_ptr + sizeof(_type))

#define _A_DOFFS(_ptr, _type, _cast) 					\
	(_cast)((char *)_ptr - sizeof(_type))

#define _A_INVALID_PTR(_ptr) 						\
	(*(_ptr)->chunk.le_prev != _ptr)

#define _A_ADD_CHUNK(_ap, _cp)						\
do {									\
	LIST_INSERT_HEAD(&(_ap)->list, _cp, chunk);			\
	(_ap)->allocated += (_cp)->size;				\
} while (0)

#define _A_DEL_CHUNK(_ap, _cp)						\
do {									\
	LIST_REMOVE(_cp, chunk);					\
	(_ap)->allocated -= (_cp)->size;				\
} while (0)


_A_GLOBAL area_t *
area_root(void (*f) (void *), void *arg)
{
	area_t *ap;

	ap = calloc(sizeof(area_t), 1);

	LIST_INIT(&ap->list);

	if (ap != NULL && f != NULL) {
		ap->err = malloc(sizeof(struct A_err));

		if (ap->err != NULL) {
			ap->err->func = f;
			ap->err->arg = arg;

			return ap;
		}
	}

	/* error */
	if (f != NULL) {
		errno = ENOMEM;
		(*f) (arg);
	}

	return NULL;
}

_A_GLOBAL area_t *
area_node(area_t *rp)
{
	area_t *ap;

	ap = calloc(sizeof(area_t), 1);

	if (ap != NULL) {
		ap->parent = rp;
		ap->err = rp->err;
		SLIST_INSERT_HEAD(&rp->nodes, ap, node);

		LIST_INIT(&ap->list);
	} else
		_A_ERROR(rp, ENOMEM);

	return ap;
}

_A_GLOBAL void
area_destroy(area_t *rp)
{
	area_t *ap, *apnxt;
	struct A_chunk *cp, *cpnxt;

	for (ap = SLIST_FIRST(&rp->nodes); ap != LIST_END(&rp->nodes);
	     ap = apnxt) {
		apnxt = SLIST_NEXT(ap, node);
		area_destroy(ap);
	}

	for (cp = LIST_FIRST(&rp->list); cp != LIST_END(&rp->list); 
	    cp = cpnxt) {
		cpnxt = LIST_NEXT(cp, chunk);
		free(cp);
	}

	if (rp->parent != NULL)
		SLIST_REMOVE(&rp->parent->nodes, rp, A_node, node);
	else
		free(rp->err);

	free(rp);
}

_A_GLOBAL size_t
area_size(area_t *ap)
{
	area_t *pp;
	size_t size;

	size = ap->allocated;

	SLIST_FOREACH(pp, &ap->nodes, node) {
		size += area_size(pp);
	}

	return size;
}

_A_GLOBAL void *
a_malloc(area_t *ap, size_t size)
{
	struct A_chunk *cp;

	cp = malloc(size + sizeof(struct A_chunk));
	_A_COND_ERROR_RET(cp == NULL, ap, ENOMEM, NULL);

	cp->size = size;
	_A_ADD_CHUNK(ap, cp);

	return _A_OFFS(cp, struct A_chunk, void *);
}

_A_GLOBAL void *
a_calloc(area_t *ap, size_t nmemb, size_t size)
{
	struct A_chunk *cp;

	cp = calloc(1, (size * nmemb) + sizeof(struct A_chunk));
	_A_COND_ERROR_RET(cp == NULL, ap, ENOMEM, NULL);

	cp->size = size;
	_A_ADD_CHUNK(ap, cp);

	return _A_OFFS(cp, struct A_chunk, void *);
}

_A_LOCAL void *
real_realloc(area_t *ap, size_t size, struct A_chunk *old)
{
	struct A_chunk *cp;

	_A_DEL_CHUNK(ap, old);

	cp = realloc(old, size + sizeof(struct A_chunk));
	_A_COND_ERROR_RET(cp == NULL, ap, ENOMEM, NULL);

	cp->size = size;
	_A_ADD_CHUNK(ap, cp);

	return _A_OFFS(cp, struct A_chunk, void *);
}

_A_GLOBAL void *
a_realloc(area_t *ap, void *ptr, size_t size)
{
	struct A_chunk *old;

	if (ptr == NULL)
		return a_malloc(ap, size);

	old = _A_DOFFS(ptr, struct A_chunk, struct A_chunk *);

	if (_A_INVALID_PTR(old)) {
		_A_ERROR(ap, EFAULT);

		return a_malloc(ap, size);
	}

	return (old->size != size) ? real_realloc(ap, size, old) : ptr;
}

_A_GLOBAL void *
a_realloc0(area_t *ap, void *ptr, size_t size)
{
	void *ret;
	struct A_chunk *old;
	size_t osize;

	if (ptr == NULL)
		return a_calloc(ap, 1, size);

	old = _A_DOFFS(ptr, struct A_chunk, struct A_chunk *);

	if (_A_INVALID_PTR(old)) {
		_A_ERROR(ap, EFAULT);

		return a_calloc(ap, 1, size);
	}

	if (old->size == size)
		return ptr;

	osize = old->size;
	ret = real_realloc(ap, size, old);

	if (ret != NULL && size > osize)
		memset((char *)ret + osize, 0, size - osize);

	return ret;
}

_A_GLOBAL int
_a_expand(area_t *ap, void **ptr, size_t size)
{
	struct A_chunk *old;

	if (*ptr != NULL) {
		old = _A_DOFFS(*ptr, struct A_chunk, struct A_chunk *);
		
		if (_A_INVALID_PTR(old)) {
			_A_ERROR(ap, EFAULT);
			
			*ptr = a_malloc(ap, size);
		} else if (old->size < size) 
			*ptr = real_realloc(ap, size, old);
	} else
		*ptr = a_malloc(ap, size);

	return (*ptr != NULL);
}

_A_GLOBAL int
_a_expand0(area_t *ap, void **ptr, size_t size)
{
	struct A_chunk *old;
	size_t osize;

	if (*ptr != NULL) {
		old = _A_DOFFS(*ptr, struct A_chunk, struct A_chunk *);

		if (_A_INVALID_PTR(old)) {
			_A_ERROR(ap, EFAULT);

			*ptr = a_calloc(ap, 1, size);
		} else 	if (old->size < size) {
			osize = old->size;
			*ptr = real_realloc(ap, size, old);

			if (*ptr != NULL && size > osize)
				memset((char *)*ptr + osize, 0, size - osize);
		} 
	} else
		*ptr = a_calloc(ap, 1, size);

	return (*ptr != NULL);
}

_A_GLOBAL void *
a_memdup(area_t *ap, const void *ptr, size_t size)
{
	void *ret;

	ret = a_malloc(ap, size);

	if (ret != NULL)
		memcpy(ret, ptr, size);

	return ret;
}

_A_GLOBAL void
_a_free(area_t *ap, void *ptr)
{
	struct A_chunk *cp;

	_A_COND_ERROR_RET(ptr == NULL, ap, EINVAL,);

	cp = _A_DOFFS(ptr, struct A_chunk, struct A_chunk *);

	_A_COND_ERROR_RET(_A_INVALID_PTR(cp), ap, EFAULT,);

	_A_DEL_CHUNK(ap, cp);
	free(cp);
}

_A_GLOBAL int
area_errfunc(area_t *ap, void (*f) (void *), void *arg)
{
	if (ap == NULL) {
		if (f != NULL) {
			errno = ENOMEM;
			(*f) (arg);
		}

		return 0;
	}

	if (ap->parent == NULL) {
		if (f == NULL) {
			if (ap->err != NULL) {
				free(ap->err);
				ap->err = NULL;
			}
		} else {
			if (ap->err == NULL) {
				ap->err = malloc(sizeof(struct A_err));

				if (ap->err == NULL)
					goto error;
			}

			ap->err->func = f;
			ap->err->arg = arg;
		}
	} else {
		if (f != NULL) {
			ap->err = a_new(ap, struct A_err);

			if (ap->err == NULL)
				goto error;

			ap->err->func = f;
			ap->err->arg = arg;
		} else
			ap->err = NULL;
	}

	return 1;

      error:
	errno = ENOMEM;
	(*f) (arg);

	return 0;
}

_A_GLOBAL int
area_relink(area_t *op, area_t *np, void *ptr)
{
	struct A_chunk *cp;

	cp = _A_DOFFS(ptr, struct A_chunk, struct A_chunk *);

	_A_COND_ERROR_RET(_A_INVALID_PTR(cp), op, EFAULT, 0);

	_A_DEL_CHUNK(op, cp);

	if (np != NULL)
		_A_ADD_CHUNK(np, cp);

	return 1;
}
