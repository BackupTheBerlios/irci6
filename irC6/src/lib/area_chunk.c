
/* $Id: area_chunk.c,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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
#include <unistd.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/tree.h>

#include <area_chunk.h>

RB_HEAD(ACHUNK_tree, ACHUNK_area);
SLIST_HEAD(ACHUNK_free_atoms_head, ACHUNK_free_atom);

struct ACHUNK_area {
	RB_ENTRY(ACHUNK_area) node;

	unsigned int index;
	unsigned int free_chunk;
};

struct ACHUNK_chunk {
	area_t *area;

	size_t atom_size;
	size_t nmemb;
#ifdef AREACHUNK_DEBUG
	unsigned int allocated_areas;
#endif

	struct ACHUNK_area *cur;
	struct ACHUNK_tree tree;

	struct ACHUNK_free_atoms_head free_atoms;
};

struct ACHUNK_free_atom {
	SLIST_ENTRY(ACHUNK_free_atom) list;
};

#define _ACHUNK_COND_RET(_cond, _ret)					\
	do { if (_cond) return _ret; } while (0)

#define _ACHUNK_OFFS(_ptr, _offs)					\
	(void *)((char *)_ptr + (_offs) +  sizeof(struct ACHUNK_area))

#define _ACHUNK_ADDR_IS_IN_THIS_AREA(_addr, _area)			\
	((void *)(_addr) < _ACHUNK_OFFS(_area, (_area)->index))

#define _ACHUNK_DELETE(_ac)						\
	(_ac)->free_chunk = -1U

#define _ACHUNK_IS_DELETED(_ac)						\
	((_ac)->free_chunk == -1U)

#ifdef AREACHUNK_DEBUG
#define _ACHUNK_ADD_AREA(_ac)						\
    	(_ac)->allocated_areas++
#define _ACHUNK_DEL_AREA(_ac)						\
    	(_ac)->allocated_areas--
#else
#define _ACHUNK_ADD_AREA(_ac)
#define _ACHUNK_DEL_AREA(_ac)
#endif

_A_LOCAL int
_tree_compare(struct ACHUNK_area *p, struct ACHUNK_area *node)
{
	if (p == node)
		return 0;
	else if (p < node)
		return -1;
	else
		return (_ACHUNK_ADDR_IS_IN_THIS_AREA(p, node)) ? 0 : 1;
}

RB_PROTOTYPE(ACHUNK_tree, ACHUNK_area, node, _tree_compare);
RB_GENERATE(ACHUNK_tree, ACHUNK_area, node, _tree_compare);

_A_GLOBAL achunk_t *
_area_chunk_new(area_t *area, size_t atom_size, size_t nmemb)
{
	achunk_t *ac;

	_ACHUNK_COND_RET(area == NULL, NULL);
	_ACHUNK_COND_RET(atom_size < sizeof(struct ACHUNK_free_atom), NULL);

	ac = a_new0(area, achunk_t);

	_ACHUNK_COND_RET(ac == NULL, NULL);

	ac->area = area;
	ac->atom_size = atom_size;
	ac->nmemb = nmemb;

	RB_INIT(&ac->tree);
	SLIST_INIT(&ac->free_atoms);

	ac->cur = a_alloc0(area, sizeof(struct ACHUNK_area) +
			   (atom_size * nmemb));

	if (ac->cur == NULL) {
		a_free(area, ac);
		return NULL;
	}

	RB_INSERT(ACHUNK_tree, &ac->tree, ac->cur);
	_ACHUNK_ADD_AREA(ac);

	return ac;
}

_A_GLOBAL void
_area_chunk_destroy_dup(achunk_t *ac)
{
	area_destroy(ac->area);
}

_A_LOCAL void
ac_clean(achunk_t *ac, void **free_atom, struct ACHUNK_area **free_area)
{
	struct ACHUNK_free_atom *fa, *fa_nxt;
	struct ACHUNK_area *tmp, *fa_area;

	*free_atom = NULL;
	*free_area = NULL;
	fa_area = NULL;

	for (fa = SLIST_FIRST(&ac->free_atoms); fa != NULL; fa = fa_nxt) {
		fa_nxt = SLIST_NEXT(fa, list);

		tmp = RB_FIND(ACHUNK_tree, &ac->tree, (struct ACHUNK_area *)fa);
		if (tmp == NULL)
			SLIST_REMOVE(&ac->free_atoms, fa,
				     ACHUNK_free_atom, list);
		else if (_ACHUNK_IS_DELETED(tmp)) {
			SLIST_REMOVE(&ac->free_atoms, fa,
				     ACHUNK_free_atom, list);

			if (*free_area != NULL) {
				RB_REMOVE(ACHUNK_tree, &ac->tree, tmp);
				a_free(ac->area, tmp);

				_ACHUNK_DEL_AREA(ac);
			} else {
				tmp->index = 0;
				tmp->free_chunk = 0;
				*free_area = tmp;
			}
		} else if (*free_atom == NULL) {
			SLIST_REMOVE(&ac->free_atoms, fa,
				     ACHUNK_free_atom, list);

			*free_atom = fa;
			fa_area = tmp;
		}
	}

	if (*free_atom != NULL) {
		if (*free_area != NULL) {
			RB_REMOVE(ACHUNK_tree, &ac->tree, *free_area);

			a_free(ac->area, *free_area);
			*free_area = NULL;
				
			_ACHUNK_DEL_AREA(ac);
		}

		*free_area = fa_area;
	}
}

_A_GLOBAL void *
achunk_new(achunk_t *ac)
{
	void *free_atom, *ret;
	struct ACHUNK_area *free_area, *area;

	ac_clean(ac, &free_atom, &free_area);

	if (free_atom) {
		free_area->free_chunk--;
		return free_atom;
	}

	if (ac->cur->index < (ac->nmemb * ac->atom_size))
		area = ac->cur;
	else if (free_area != NULL) {
		area = free_area;
		free_area = NULL;
	} else {
		ac->cur = a_alloc0(ac->area, sizeof(struct ACHUNK_area) +
				   (ac->atom_size * ac->nmemb));
		_ACHUNK_COND_RET(ac->cur == NULL, NULL);

		RB_INSERT(ACHUNK_tree, &ac->tree, ac->cur);
		_ACHUNK_ADD_AREA(ac);

		area = ac->cur;
	}

	ret = _ACHUNK_OFFS(area, area->index);
	area->index += ac->atom_size;

	return ret;
}

_A_GLOBAL void *
achunk_new0(achunk_t *ac)
{
	void *mem;

	mem = achunk_new(ac);
	if (mem != NULL)
		memset(mem, 0, ac->atom_size);

	return mem;
}

_A_GLOBAL int
achunk_del(achunk_t *ac, void *mem)
{
	struct ACHUNK_area *area;

	area = RB_FIND(ACHUNK_tree, &ac->tree, (struct ACHUNK_area *)mem);

	if (area == NULL || _ACHUNK_IS_DELETED(area))
		return 0;

	area->free_chunk++;

	if (area->free_chunk == ac->nmemb || 
	    (area->free_chunk * ac->atom_size) == area->index)
		_ACHUNK_DELETE(area);

	SLIST_INSERT_HEAD(&ac->free_atoms, (struct ACHUNK_free_atom *)mem,
			  list);

	return 1;
}

_A_GLOBAL int
achunk_del0(achunk_t *ac, void *mem)
{
	struct ACHUNK_area *area;

	area = RB_FIND(ACHUNK_tree, &ac->tree, (struct ACHUNK_area *)mem);

	if (area == NULL || _ACHUNK_IS_DELETED(area))
		return 0;

	memset(mem, 0, ac->atom_size);
	area->free_chunk++;

	if (area->free_chunk == ac->nmemb || 
	    (area->free_chunk * ac->atom_size) == area->index)
		_ACHUNK_DELETE(area);

	SLIST_INSERT_HEAD(&ac->free_atoms, (struct ACHUNK_free_atom *)mem,
			  list);

	return 1;
}

_A_GLOBAL void
area_chunk_clean(achunk_t *ac)
{
	void *free_atom;
	struct ACHUNK_area *free_area;

	ac_clean(ac, &free_atom, &free_area);

	if (free_atom == NULL && free_area != NULL) {
		RB_REMOVE(ACHUNK_tree, &ac->tree, free_area);
		a_free(ac->area, free_area);
			
		_ACHUNK_DEL_AREA(ac);
	}
}

#ifdef AREACHUNK_DEBUG
_A_GLOBAL unsigned int
area_chunk_allocated_areas(achunk_t *ac)
{
	return ac->allocated_areas;
}
#endif
