
/* $Id: area.h,v 1.1 2005/04/16 09:23:27 benkj Exp $ */

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

#ifndef _AREA_H
#define _AREA_H

#include <sys/types.h>

#ifdef __GNUC__
#define _A_GLOBAL	__inline
#define _A_LOCAL	static __inline
#else
#define _A_GLOBAL
#define _A_LOCAL	static
#endif

typedef struct A_node area_t;
typedef void (*area_errfunc_t) (void *);

area_t *area_root(void (*)(void *), void *);
area_t *area_node(area_t *);
void area_destroy(area_t *);
int area_relink(area_t *, area_t *, void *);
size_t area_size(area_t *);
int area_errfunc(area_t *, void (*)(void *), void *);
void *a_malloc(area_t *, size_t);
void *a_calloc(area_t *, size_t, size_t);
void *a_realloc(area_t *, void *, size_t);
void *a_realloc0(area_t *, void *, size_t);
int _a_expand(area_t *, void **, size_t);
int _a_expand0(area_t *, void **, size_t);
void *a_memdup(area_t *, const void *, size_t);
void _a_free(area_t *, void *);

#define a_alloc(a, x)		a_malloc(a, x)
#define a_alloc0(a, x)		a_calloc(a, x, 1)
#define a_nalloc(a, x, y)	a_malloc(a, (x) * (y))
#define a_nalloc0(a, x, y)	a_calloc(a, x, y)
#define a_new(a, t)		(t *)a_malloc(a, sizeof(t))
#define a_new0(a, t)		(t *)a_calloc(a, 1, sizeof(t))
#define a_nnew(a, x, t)		a_malloc(a, (x) * sizeof(t))
#define a_nnew0(a, x, t)	a_calloc(a, x, sizeof(t))
#define a_nrealloc(a, p, x, y)	a_realloc(a, p, (x) * (y))
#define a_nrealloc0(a, p, x, y)	a_realloc0(a, p, (x) * (y))
#define a_expand(a, p, x)	_a_expand(a, (void **)(p), (x))
#define a_expand0(a, p, x)	_a_expand0(a, (void **)(p), (x))
#define a_nexpand(a, p, x, y)	_a_expand(a, (void **)(p), (x) * (y))
#define a_nexpand0(a, p, x, y)	_a_expand0(a, (void **)(p), (x) * (y))
#define a_strdup(a, s)		((char *)a_memdup(a, s, strlen(s) + 1))

#define a_free(a, p) 		do { _a_free(a, p); p = NULL; } while (0)

#define A_REALLOC(a, p, x)	(p = a_realloc(a, p, x))	
#define A_REALLOC0(a, p, x)	(p = a_realloc0(a, p, x))	
#define A_NREALLOC(a, p, x, y)	(p = a_nrealloc(a, p, x, y))	
#define A_NREALLOC0(a, p, x, y)	(p = a_nrealloc0(a, p, x, y))	



#ifndef AREA_NO_DEF
area_t *__AREA_def;

#define adef_init()			__AREA_def = NULL

#define area_setdef(_area)		__AREA_def = _area
#define area_unsetdef()			__AREA_def = NULL
#define area_getdef()			__AREA_def

#define adef_malloc(x)			a_malloc(__AREA_def, x)
#define adef_calloc(x, y)		a_calloc(__AREA_def, x, y)
#define adef_realloc(p, x)		a_realloc(__AREA_def, p, x)
#define adef_realloc0(p, x)		a_realloc0(__AREA_def, p, x)
#define adef_expand(p, x)		a_expand(__AREA_def, p, x)
#define adef_expand0(p, x)		a_expand0(__AREA_def, p, x)
#define adef_memdup(p, x)		a_memdup(__AREA_def, p, x)
#define adef_free(p)			a_free(__AREA_def, p)
#define adef_alloc(x)			a_alloc(__AREA_def, x)
#define adef_alloc0(x)			a_alloc0(__AREA_def, x)
#define adef_nalloc(x, y)		a_nalloc(__AREA_def, x, y)
#define adef_nalloc0(x, y)		a_nalloc0(__AREA_def, x, y)
#define adef_new(t)			a_new(__AREA_def, t)
#define adef_new0(t)			a_new0(__AREA_def, t)
#define adef_nnew(x, t)			a_nnew(__AREA_def, x, t)
#define adef_nnew0(x, t)		a_nnew0(__AREA_def, x, t)
#define adef_nrealloc(p, x, y)		a_nrealloc(__AREA_def, p, x, y)
#define adef_nrealloc0(p, x, y)		a_nrealloc0(__AREA_def, p, x, y)
#define adef_nexpand(p, x, y)		a_nexpand(__AREA_def, p, x, y)
#define adef_nexpand0(p, x, y)		a_nexpand0(__AREA_def, p, x, y)
#define adef_strdup(s)			a_strdup(__AREA_def, s)

#define ADEF_REALLOC(p, x)		A_REALLOC(__AREA_def, p, x)	
#define ADEF_REALLOC0(p, x)		A_REALLOC0(__AREA_def, p, x)	
#define ADEF_NREALLOC(p, x, y)		A_NREALLOC(__AREA_def, p, x, y)	
#define ADEF_NREALLOC0(p, x, y)		A_NREALLOC0(__AREA_def, p, x, y)	

#endif
#endif
