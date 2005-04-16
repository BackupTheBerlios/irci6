
/* $Id: hash.h,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

/*
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

#ifndef __HASH_H
#define __HASH_H

#include <area.h>

/* options */
#define HASHTABLE_NOOPT		0x0
#define HASHTABLE_DUPDATA	0x100
#define HASHTABLE_ALLOWCOLL	0x200
#define HASHTABLE_CHMATCH	0x400
#define HASHTABLE_DUPASCOLL	0x800
#define HASHTABLE_CACHEMASK	0x7f

/* error */
#define HASH_OK		1
#define HASH_DUP	0
#define HASH_ECOLL	-1
#define HASH_ENOTFOUND	-2
#define HASH_UNSPEC	-63
#define HASH_EINVAL	-126
#define HASH_EMEM	-127

typedef struct hashtable_struct hashtable_t;
typedef struct hash_struct hash_t;

typedef struct {
	void *data;
	size_t size;
	void *arg;
	unsigned int hash;
} hash_collision_t;

#define HASHTABLE_FOREACH(_ht, _hi)					\
    for (hashtable_begin(_ht, _hi); !hashtable_end(_ht, _hi); 		\
	hashtable_next(_ht, _hi))

#define HASHCOLLISION_FOREACH(_ht, _hi)					\
    for (; !hashcollision_end(_ht, _hi); hashcollision_next(_ht, _hi))

#define hashtable_add_d(_ht, _data, _size, _arg)			\
    hashtable_wrap(_ht, _data, _size, _arg, NULL, hashtable_add)
#define hashtable_find_d(_ht, _data, _size)	 			\
    hashtable_wrap(_ht, _data, _size, NULL, NULL, hashtable_find)
#define hashtable_del_d(_ht, _data, _size)	 			\
    hashtable_wrap(_ht, _data, _size, NULL, NULL, hashtable_del)
#define hashtable_add_h(_ht, _data, _size, _arg, _hi)			\
    hashtable_wrap(_ht, _data, _size, _arg, _hi,  hashtable_add)
#define hashtable_find_h(_ht, _data, _size, _hi)	 		\
    hashtable_wrap(_ht, _data, _size, NULL, _hi, hashtable_find)
#define hashtable_del_h(_ht, _data, _size, _hi)	 			\
    hashtable_wrap(_ht, _data, _size, NULL, _hi, hashtable_del)

#define hashtable_destroy(_ht)						\
    do { _hashtable_destroy(_ht); _ht = NULL; } while (0)
#define hash_destroy(_ht, _hi)						\
    do { _hash_destroy(_ht, _hi); _hi = NULL; } while (0)
#define hash_collisions_destroy(_ht, _hc)				\
    do { _hash_collisions_destroy(_ht, _hc); _hc = NULL; } while (0)
    
#define hash_init(_hi)							\
    do { (_hi) = NULL; } while (0)
#define hash_getarg(_hi, _arg) 						\
    hash_get(_hi, NULL, NULL, NULL, (void **)(_arg))
#define hash_newvoid(_ht)						\
    hash_new(_ht, NULL, 0, NULL)

/* hash.c */
hashtable_t *hashtable_new(area_t *, unsigned int);
void _hashtable_destroy(hashtable_t *);
int hashtable_add(hashtable_t *, hash_t *);
int hashtable_find(hashtable_t *, hash_t *);
int hashtable_find_id(hashtable_t *, unsigned int, hash_collision_t **);
int hashtable_del(hashtable_t *, hash_t *);
int hashtable_wrap(hashtable_t *, void *, size_t, void *, hash_t **,
		   int (*func) (hashtable_t *, hash_t *));
void hash_set(hash_t *, void *, size_t, void *);
hash_t *hash_new(hashtable_t *, void *, size_t, void *);
hash_t *hash_dup(hashtable_t *, hash_t *);
int hash_get(hash_t *, unsigned int *, void **, size_t *, void **);
void _hash_destroy(hashtable_t *, hash_t *);
int hash_collisions(hashtable_t *, hash_t *, hash_collision_t **);
void _hash_collisions_destroy(hashtable_t *, hash_collision_t *);
void hashtable_begin(hashtable_t *, hash_t *);
int hashtable_end(hashtable_t *, hash_t *);
void hashtable_next(hashtable_t *, hash_t *);
int hashcollision_end(hashtable_t *, hash_t *);
int hashcollision_next(hashtable_t *, hash_t *);

#endif
