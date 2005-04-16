
/* $Id: area_chunk.h,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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

#ifndef _AREA_CHUNK_H
#define _AREA_CHUNK_H

#include <area.h>

typedef struct ACHUNK_chunk achunk_t;

#define area_chunk_new(_area, _type, _nmemb)				\
	_area_chunk_new(_area, sizeof(_type), _nmemb)
#define area_chunk_new_size(_area, _size, _nmemb)			\
	_area_chunk_new(_area, _size, _nmemb)
#define area_chunk_new_dup(_area, _type, _nmemb)			\
	_area_chunk_new(area_node(_area), sizeof(_type), _nmemb)
#define area_chunk_new_size_dup(_area, _size, _nmemb)			\
	_area_chunk_new(area_node(_area), _size, _nmemb)
#define area_chunk_destroy_dup(_ac)					\
	do { _area_chunk_destroy_dup(_ac); _ac = NULL; } while (0)

achunk_t *_area_chunk_new(area_t *, size_t, size_t);
void _area_chunk_destroy_dup(achunk_t *);
void *achunk_new(achunk_t *);
void *achunk_new0(achunk_t *);
int achunk_del(achunk_t *, void *);
int achunk_del0(achunk_t *, void *);
void area_chunk_clean(achunk_t *);
#ifdef AREACHUNK_DEBUG
unsigned int area_chunk_allocated_areas(achunk_t *);
#endif
#endif
