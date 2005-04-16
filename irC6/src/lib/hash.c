
/* $Id: hash.c,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/tree.h>
#include <sys/queue.h>
#include <errno.h>

#include <hash.h>
#include <area_chunk.h>

RB_HEAD(hash_tree, hash_tree_node);
SLIST_HEAD(hash_list, hash_list_node);

struct hash_tree_node {
	RB_ENTRY(hash_tree_node) tree;

	unsigned int first_hash;

	size_t collisions;
	size_t elms;
	struct hash_list list;
};

struct hash_list_node {
	SLIST_ENTRY(hash_list_node) list;

	unsigned int hash;
	unsigned int dup;

	void *data;
	size_t size;

	void *arg;
};

struct cache_entry {
	struct hash_list_node *lnode;
	struct hash_tree_node *tnode;
	int used;
};

struct hashtable_struct {
	area_t *area;
	achunk_t *list_chunk;
	achunk_t *tree_chunk;
	achunk_t *hash_chunk;
	size_t elms;
	struct hash_tree tree;

	unsigned int dupdata:1;
	unsigned int allowcoll:1;
	unsigned int chmatch:1;
	unsigned int dupascoll:1;
	unsigned int cache_size;

	struct cache_entry *cache;
	size_t cache_offs;
};

struct hash_struct {
	void *data;
	size_t size;

	void *arg;

	unsigned int hash;

	struct hash_list_node *lnode;
	struct hash_tree_node *tnode;

	int ret;

	struct hash_list_node *collnode;
};

static unsigned int _fnv_hash(void *, size_t);
static int _oat_hash(void *, size_t);

#define _HI_INVALID_PTR(_hi) 						\
	((_hi)->lnode == NULL || (_hi)->tnode == NULL)

#define _HI_RESET_PTR(_hi) do { 					\
	(_hi)->lnode = NULL;						\
	(_hi)->tnode = NULL; 						\
} while (0)

#define _HI_CLEAN(_ht, _hi)  do {					\
	if ((_hi)->collnode != NULL) {					\
		_HT_FREE_ENTRY(_ht, (_hi)->collnode);			\
		(_hi)->collnode = NULL;					\
	}								\
} while (0)

#define _HT_FREE_ENTRY(_ht, _list) do {					\
	if ((_ht)->dupdata)						\
		a_free((_ht)->area, (_list)->data);			\
	achunk_del((_ht)->list_chunk, _list);				\
} while (0)

static int
_tree_compare(struct hash_tree_node *a, struct hash_tree_node *b)
{
	if (a->first_hash < b->first_hash)
		return -1;
	else if (a->first_hash > b->first_hash)
		return 1;
	else
		return 0;
}

RB_PROTOTYPE(hash_tree, hash_tree_node, tree, _tree_compare);
RB_GENERATE(hash_tree, hash_tree_node, tree, _tree_compare);

hashtable_t *
hashtable_new(area_t *area, unsigned int opt)
{
	hashtable_t *res;
	area_t *node;

	if ((node = area_node(area)) == NULL)
		return NULL;

	if ((res = a_new0(node, hashtable_t)) == NULL)
		            return NULL;

	res->area = node;
	res->list_chunk = area_chunk_new(res->area, struct hash_list_node, 64);
	res->tree_chunk = area_chunk_new(res->area, struct hash_tree_node, 64);
	res->hash_chunk = area_chunk_new(res->area, hash_t, 64);

	if (res->list_chunk == NULL || res->tree_chunk == NULL ||
	    res->hash_chunk == NULL)
		goto error;

	RB_INIT(&res->tree);

	if (opt & HASHTABLE_DUPDATA)
		res->dupdata = 1;
	if (opt & HASHTABLE_ALLOWCOLL)
		res->allowcoll = 1;
	if (opt & HASHTABLE_CHMATCH)
		res->chmatch = 1;
	if (opt & HASHTABLE_DUPASCOLL)
		res->dupascoll = 1;
	if (opt & HASHTABLE_CACHEMASK) {
		res->cache_size = opt & HASHTABLE_CACHEMASK;

		res->cache = a_nnew0(node, res->cache_size, struct cache_entry);

		if (res->cache == NULL)
			goto error;
	}

	return res;

      error:
	area_destroy(node);
	return NULL;
}

void
_hashtable_destroy(hashtable_t *hash)
{
	if (hash != NULL)
		area_destroy(hash->area);
}

#define _CMP_LIST(a, b)							\
	((a)->size == (b)->size && !memcmp((a)->data, (b)->data, (a)->size))
#define _CMP_DATA(data1, size1, data2, size2)				\
	(size1 == size2 && !memcmp(data1, data2, size1))
#define _CMP_DATALIST(a, bdata, bsize)					\
	((a)->size == bsize && !memcmp((a)->data, bdata, bsize))


/*
 * CACHE OPERATIONS 
 */

static void
_add_cache(hashtable_t *ht, struct hash_tree_node *tree,
	   struct hash_list_node *list)
{
	if (ht->cache_size > 0) {
		ht->cache[ht->cache_offs].tnode = tree;
		ht->cache[ht->cache_offs].lnode = list;
		ht->cache[ht->cache_offs].used = 1;

		ht->cache_offs = (ht->cache_offs + 1) % ht->cache_size;
	}
}

static int
_search_in_cache(hashtable_t *ht, hash_t *hi)
{
	unsigned int i;

	if (ht->cache_size <= 0)
		return 0;

	for (i = ht->cache_offs;
	     i != (ht->cache_offs - 1) % ht->cache_size;
	     i = (i + 1) % ht->cache_size) {
		if (!ht->cache[i].used || ht->cache[i].lnode->hash != hi->hash)
			continue;

		if (ht->chmatch &&
		    !_CMP_DATALIST(ht->cache[i].lnode, hi->data, hi->size))
			continue;

		hi->tnode = ht->cache[i].tnode;
		hi->lnode = ht->cache[i].lnode;

		return 1;
	}

	return 0;
}

static void
_del_cache(hashtable_t *ht, hash_t *hi)
{
	unsigned int i;

	if (ht->cache_size <= 0)
		return;

	for (i = ht->cache_offs;
	     i != (ht->cache_offs - 1) % ht->cache_size;
	     i = (i + 1) % ht->cache_size) {
		if (!ht->cache[i].used)
			continue;

		if (ht->cache[i].lnode == hi->lnode)
			ht->cache[i].used = 0;
	}
}


/*
 * SETUP HASH_T
 */

static void
_hinfo_remake(hash_t *hi, int ret)
{
	hi->data = hi->lnode->data;
	hi->size = hi->lnode->size;
	hi->arg = hi->lnode->arg;

	hi->hash = hi->tnode->first_hash;

	hi->ret = ret;
}

static void
_hinfo_reset(hash_t *hi, struct hash_tree_node *tree,
	     struct hash_list_node *list, int ret)
{
	if (list == NULL || tree == NULL)
		_HI_RESET_PTR(hi);
	else {
		hi->lnode = list;
		hi->tnode = tree;

		_hinfo_remake(hi, ret);
	}
}

void
hash_set(hash_t *hi, void *data, size_t size, void *arg)
{
	if (hi == NULL)
		return;

	if (data != NULL && size != 0) {
		hi->data = data;
		hi->size = size;
		hi->hash = _fnv_hash(data, size);
		hi->arg = arg;
	}

	_HI_RESET_PTR(hi);
	hi->ret = HASH_UNSPEC;
}

hash_t *
hash_new(hashtable_t *ht, void *data, size_t size, void *arg)
{
	hash_t *hi;

	if (ht == NULL)
		return NULL;

	hi = achunk_new0(ht->hash_chunk);
	hash_set(hi, data, size, arg);

	return hi;
}

hash_t *
hash_dup(hashtable_t *ht, hash_t *h)
{
	hash_t *hi;

	if (ht == NULL || h == NULL)
		return NULL;

	hi = achunk_new0(ht->hash_chunk);
	hash_set(hi, h->data, h->size, h->arg);

	return hi;
}

int
hash_get(hash_t *hi, unsigned int *hash, void **data, size_t *size, void **arg)
{
	if (hi == NULL)
		return 0;

	if (data != NULL)
		*data = hi->data;
	if (size != NULL)
		*size = hi->size;
	if (hash != NULL)
		*hash = hi->hash;
	if (arg != NULL)
		*arg = hi->arg;

	return 1;
}

void
_hash_destroy(hashtable_t *ht, hash_t *hi)
{
	if (ht == NULL || hi == NULL)
		return;

	achunk_del(ht->hash_chunk, hi);
}

/*
 * ADD NEW HASH IN TABLE
 */

static int
_insert_collision(hashtable_t *ht, hash_t *hi, struct hash_list_node *coll)
{
	int dup = _CMP_LIST(hi->lnode, coll);

	/* don't return error if collisions are permitted */
	if (ht->allowcoll) {
		if (dup && !ht->dupascoll) {
			_HT_FREE_ENTRY(ht, hi->lnode);
			hi->lnode = coll;

			coll->dup++;
		} else {
			hi->tnode->collisions++;
			SLIST_INSERT_AFTER(coll, hi->lnode, list);
		}
	} else {
		/* if word collides save *list in hi->collnode */
		if (dup)
			_HT_FREE_ENTRY(ht, hi->lnode);
		else
			hi->collnode = hi->lnode;
		hi->lnode = coll;
	}

	return (dup) ? HASH_DUP : HASH_ECOLL;
}

static int
_insert_entry(hashtable_t *ht, hash_t *hi)
{
	struct hash_list_node *lent, *oldent = NULL;

	SLIST_FOREACH(lent, &hi->tnode->list, list) {
		if (hi->lnode->hash == lent->hash)
			return _insert_collision(ht, hi, lent);

		/* insert sorted hash */
		if (hi->lnode->hash < lent->hash) {
			if (oldent != NULL)
				SLIST_INSERT_AFTER(oldent, hi->lnode, list);
			else
				SLIST_INSERT_HEAD(&hi->tnode->list,
						  hi->lnode, list);

			return HASH_OK;
		}

		oldent = lent;
	}

	SLIST_INSERT_AFTER(oldent, hi->lnode, list);

	return HASH_OK;
}

static struct hash_tree_node *
_setup_node(hashtable_t *ht, hash_t *hi)
{
	struct hash_tree_node find, *tree;

	find.first_hash = hi->hash;

	tree = RB_FIND(hash_tree, &ht->tree, &find);

	if (tree == NULL) {
		tree = achunk_new0(ht->tree_chunk);

		if (tree != NULL) {
			tree->first_hash = find.first_hash;

			RB_INSERT(hash_tree, &ht->tree, tree);
		}
	}

	return tree;
}

static struct hash_list_node *
_setup_entry(hashtable_t *ht, hash_t *hi)
{
	struct hash_list_node *list;

	list = achunk_new0(ht->list_chunk);
	if (list == NULL)
		return NULL;

	if (ht->dupdata) {
		list->data = a_memdup(ht->area, hi->data, hi->size);

		if (list->data == NULL)
			return NULL;
	} else
		list->data = hi->data;

	list->size = hi->size;
	list->arg = hi->arg;

	return list;
}

int
hashtable_add(hashtable_t *ht, hash_t *hi)
{
	struct hash_list_node *first;
	int ret = HASH_OK;

	if (ht == NULL || hi == NULL || hi->data == NULL || hi->size == 0)
		return HASH_EINVAL;

	_HI_CLEAN(ht, hi);

	/* setup structures */
	hi->tnode = _setup_node(ht, hi);
	hi->lnode = _setup_entry(ht, hi);

	if (hi->tnode == NULL || hi->lnode == NULL)
		return HASH_EMEM;

	if ((first = SLIST_FIRST(&hi->tnode->list)) != NULL) {
		if (first->hash == 0)
			first->hash = _oat_hash(first->data, first->size);

		hi->lnode->hash = _oat_hash(hi->lnode->data, hi->lnode->size);

		ret = _insert_entry(ht, hi);
	} else {
		hi->lnode->hash = 0;

		SLIST_INSERT_HEAD(&hi->tnode->list, hi->lnode, list);
	}

	_add_cache(ht, hi->tnode, hi->lnode);
	_hinfo_remake(hi, ret);

	if (ret == HASH_OK || ht->allowcoll)
		hi->tnode->elms++;

	return (ht->allowcoll) ? HASH_OK : ret;
}

/*
 * SEARCH ENTRY
 */

static int
_check_entry(hashtable_t *ht, hash_t *hi, unsigned int hash2,
	     struct hash_list_node *lent, struct hash_list_node *prev,
	     struct hash_list_node *next)
{
	if (!ht->allowcoll) {
		if (!ht->chmatch || _CMP_DATALIST(lent, hi->data, hi->size)) {
			hi->lnode = lent;

			return HASH_OK;
		}
	} else {
		if (!ht->dupascoll && _CMP_DATALIST(lent, hi->data, hi->size)) {
			hi->lnode = lent;

			return HASH_DUP;
		} else if (hi->tnode->collisions > 0) {
			if (next == NULL || next->hash != hash2) {
				if (prev != NULL && prev->hash == hash2)
					lent = prev;
				else
					lent = NULL;
			} else
				lent = next;

			if (lent != NULL &&
			    _CMP_DATALIST(lent, hi->data, hi->size)) {
				hi->lnode = lent;

				return HASH_ECOLL;
			}
		}
	}

	return HASH_ENOTFOUND;
}

static int
_search_in_table(hashtable_t *ht, hash_t *hi)
{
	struct hash_tree_node find;
	struct hash_list_node *lent, *prev = NULL;
	unsigned int hash2;
	int ret;

	if (_search_in_cache(ht, hi))
		return HASH_OK;

	find.first_hash = hi->hash;

	if ((hi->tnode = RB_FIND(hash_tree, &ht->tree, &find)) == NULL)
		return HASH_ENOTFOUND;

	if (hi->tnode->elms == 1) {
		hi->lnode = SLIST_FIRST(&hi->tnode->list);

		if (!ht->chmatch)
			return HASH_OK;
		else
			return _CMP_DATALIST(hi->lnode, hi->data, hi->size) ?
			    HASH_OK : HASH_ENOTFOUND;
	}

	hash2 = _oat_hash(hi->data, hi->size);

	SLIST_FOREACH(lent, &hi->tnode->list, list) {
		if (lent->hash == hash2) {
			ret = _check_entry(ht, hi, hash2, lent, prev,
					   SLIST_NEXT(lent, list));

			if (ret != HASH_ENOTFOUND)
				return ret;
		}

		prev = lent;
	}

	return HASH_ENOTFOUND;
}

int
hashtable_find(hashtable_t *ht, hash_t *hi)
{
	int ret;

	if (ht == NULL || hi == NULL)
		return HASH_EINVAL;

	ret = _search_in_table(ht, hi);

	_HI_CLEAN(ht, hi);
	_hinfo_reset(hi, hi->tnode, hi->lnode, ret);

	if (ret == HASH_ENOTFOUND)
		return HASH_ENOTFOUND;

	_add_cache(ht, hi->tnode, hi->lnode);

	return HASH_OK;
}

int
hashtable_find_id(hashtable_t *ht, unsigned int id, hash_collision_t **coll)
{
	struct hash_tree_node find, *tree;
	struct hash_list_node *lent;
	hash_collision_t *hc;

	find.first_hash = id;

	if ((tree = RB_FIND(hash_tree, &ht->tree, &find)) == NULL)
		return 0;

	if ((*coll = a_nnew(ht->area, tree->elms, hash_collision_t)) == NULL)
		                 return 0;

	hc = *coll;

	SLIST_FOREACH(lent, &tree->list, list) {
		hc->data = lent->data;
		hc->size = lent->size;
		hc->arg = lent->arg;
		hc->hash = lent->hash;

		hc++;
	}

	return tree->elms;
}

/*
 * DELETE ENTRY
 */

int
hashtable_del(hashtable_t *ht, hash_t *hi)
{
	int ret;

	if (ht == NULL || hi == NULL || hi->data == NULL || hi->size == 0)
		return HASH_EINVAL;

	_HI_CLEAN(ht, hi);

	if (_HI_INVALID_PTR(hi)) {
		ret = _search_in_table(ht, hi);
		_hinfo_reset(hi, hi->tnode, hi->lnode, ret);
	} else
		ret = hi->ret;

	switch (ret) {
	case HASH_DUP:
		if (hi->lnode->dup > 0) {
			hi->lnode->dup--;
			hi->tnode->elms--;

			return HASH_OK;
		}

		if (ht->dupascoll)
			ret = HASH_ECOLL;
	case HASH_ECOLL:
	case HASH_OK:
		SLIST_REMOVE(&hi->tnode->list, hi->lnode, hash_list_node, list);
		_del_cache(ht, hi);

		_HT_FREE_ENTRY(ht, hi->lnode);
		hi->lnode = NULL;

		if (--hi->tnode->elms == 0) {
			RB_REMOVE(hash_tree, &ht->tree, hi->tnode);

			achunk_del(ht->tree_chunk, hi->tnode);
			hi->tnode = NULL;
		} else if (ret == HASH_ECOLL)
			hi->tnode->collisions--;

		_hinfo_reset(hi, hi->tnode, hi->lnode, ret);

		return HASH_OK;
	case HASH_ENOTFOUND:
	default:
		return HASH_ENOTFOUND;
	}
}

/*
 * WRAPPER
 */

static hash_t *
_setup_hinfo(hashtable_t *ht, void *data, size_t size, void *arg, hash_t **hi)
{
	if (hi != NULL && *hi != NULL) {
		if (data != NULL && size != 0)
			hash_set(*hi, data, size, arg);

		return *hi;
	}

	return hash_new(ht, data, size, arg);
}

int
hashtable_wrap(hashtable_t *ht, void *data, size_t size, void *arg,
	       hash_t **hi, int (*func) (hashtable_t *, hash_t *))
{
	hash_t *hh;
	int ret;

	if (ht == NULL || func == NULL)
		return HASH_EINVAL;

	if ((hh = _setup_hinfo(ht, data, size, arg, hi)) == NULL)
		return HASH_EMEM;

	if (hh->data == NULL || hh->size == 0) {
		if (hi == NULL || *hi == NULL)
			hash_destroy(ht, hh);

		return HASH_EINVAL;
	}

	ret = (*func) (ht, hh);

	if (hi == NULL)
		hash_destroy(ht, hh);
	else if (*hi == NULL)
		*hi = hh;

	return ret;
}

/* 
 * COLLISION HANDLING
 */

int
hash_collisions(hashtable_t *ht, hash_t *hi, hash_collision_t **coll)
{
	struct hash_list_node *en, *begin = NULL, *end = NULL;
	hash_collision_t *ptr;
	size_t entries = 0;
	unsigned int hash2;

	if (ht == NULL || hi == NULL || hi->data == NULL || hi->size == 0)
		return -1;

	if (hi->collnode != NULL) {
		if ((*coll = a_nnew0(ht->area, 2, hash_collision_t)) == NULL)
			                 return 0;

		ptr = *coll;
		ptr->data = hi->data;
		ptr->size = hi->size;
		ptr->hash = _oat_hash(hi->data, hi->size);
		ptr->arg = hi->arg;

		ptr++;

		ptr->data = hi->collnode->data;
		ptr->size = hi->collnode->size;
		ptr->hash = hi->collnode->hash;
		ptr->arg = hi->collnode->arg;

		return 2;
	}

	if (hi->tnode->collisions == 0)
		return 0;

	hash2 = _oat_hash(hi->data, hi->size);

	SLIST_FOREACH(en, &hi->tnode->list, list) {
		if (en->hash == hash2) {
			if (begin == NULL)
				begin = en;
			end = en;
			entries++;
		}
	}

	if (begin == NULL || entries != hi->tnode->collisions + 1)
		return -1;

	ptr = *coll = a_nnew0(ht->area, entries, hash_collision_t);

	do {
		ptr->data = begin->data;
		ptr->size = begin->size;
		ptr->hash = begin->hash;
		ptr->arg = begin->arg;
		ptr++;

		if (begin == end)
			break;

		begin = SLIST_NEXT(begin, list);
	} while (1);

	return (int)entries;
}

void
_hash_collisions_destroy(hashtable_t *ht, hash_collision_t *coll)
{
	if (ht != NULL || coll != NULL)
		a_free(ht->area, coll);
}

static int
_next_collision(hashtable_t *ht, hash_t *hi, struct hash_list_node **lent)
{
	if (ht == NULL || hi == NULL || hi->lnode == NULL)
		return 0;

	if ((*lent = SLIST_NEXT(hi->lnode, list)) == NULL)
		return 0;

	return (hi->lnode->hash == (*lent)->hash);
}

int
hashcollision_end(hashtable_t *ht, hash_t *hi)
{
	struct hash_list_node *lent;

	return !_next_collision(ht, hi, &lent);
}

int
hashcollision_next(hashtable_t *ht, hash_t *hi)
{
	struct hash_list_node *lent;
	int ret;

	_HI_CLEAN(ht, hi);

	ret = _next_collision(ht, hi, &lent);
	hi->lnode = lent;

	if (!_HI_INVALID_PTR(hi))
		_hinfo_remake(hi, HASH_OK);

	return ret;
}

/*
 * TRAVERSING TABLE
 */
void
hashtable_begin(hashtable_t *ht, hash_t *hi)
{
	if (ht == NULL || hi == NULL)
		return;

	_HI_CLEAN(ht, hi);

	hi->tnode = RB_MIN(hash_tree, &ht->tree);
	hi->lnode = (hi->tnode != NULL) ? SLIST_FIRST(&hi->tnode->list) : NULL;

	if (!_HI_INVALID_PTR(hi))
		_hinfo_remake(hi, HASH_OK);
}

int
hashtable_end(hashtable_t *ht, hash_t *hi)
{
	if (ht != NULL && hi != NULL && hi->lnode != NULL && hi->tnode != NULL)
		return 0;
	else
		return 1;
}

void
hashtable_next(hashtable_t *ht, hash_t *hi)
{
	if (ht == NULL || hi == NULL)
		return;

	_HI_CLEAN(ht, hi);

	if (hi->lnode != NULL)
		hi->lnode = SLIST_NEXT(hi->lnode, list);

	if (hi->lnode == NULL) {
		hi->tnode = RB_NEXT(hash_tree, &ht->tree, hi->tnode);

		if (hi->tnode != NULL)
			hi->lnode = SLIST_FIRST(&hi->tnode->list);
	}

	if (!_HI_INVALID_PTR(hi))
		_hinfo_remake(hi, HASH_OK);
}

/*
 * Fowler/Noll/Vo hash
 *
 * The basis of this hash algorithm was taken from an idea sent
 * as reviewer comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 * In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 * improved on their algorithm.  Some people tried this hash
 * and found that it worked rather well.  In an EMail message
 * to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low
 * collision rate. The FNV speed allows one to quickly hash lots
 * of data while maintaining a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 * for more details as well as other forms of the FNV hash.
 ***
 *
 * NOTE: The FNV-0 historic hash is not recommended.  One should use
 *       the FNV-1 hash instead.
 *
 * To use the 32 bit FNV-0 historic hash, pass FNV0_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 *
 * To use the recommended 32 bit FNV-1 hash, pass FNV1_32_INIT as the
 * Fnv32_t hashval argument to fnv_32_buf() or fnv_32_str().
 *
 ***
 *
 * Please do not copyright this code.  This code is in the public domain.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * By:
 *      chongo <Landon Curt Noll> /\oo/\
 *      http://www.isthe.com/chongo/
 *
 * Share and Enjoy!     :-)
 */

#define FNV32_prime 16777619U
#define FNV32_init  2166136261U

static unsigned int
_fnv_hash(void *d, size_t s)
{
	unsigned int h = FNV32_init;
	size_t i = 0;
	char *p = (char *)d;

	for (; i < s; i++) {
		h = h * FNV32_prime;
		h = h ^ p[i];
	}

	return h;
}

/* on-at-a-time hash */

static int
_oat_hash(void *d, size_t len)
{
	size_t hash, i;
	char *key = (char *)d;

	for (hash = 0, i = 0; i < len; ++i) {
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}

	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);

	return hash;
}
