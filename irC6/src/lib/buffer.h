
/* $Id: buffer.h,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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

#ifndef __BUFFER_H
#define __BUFFER_H

#include <area.h>

typedef struct {
	area_t *area;
	size_t size;
#ifndef __STRICT_ANSI__
	char data[0];
#endif
} buffer_t;


#define buf_data(_buf)							\
	((char *)(_buf) + sizeof(buffer_t))
#define buf_free(_buf) 							\
	do { _buf_free(_buf); _buf = NULL; } while (0)
#define buf_insert(_to, _from, _pos) 					\
	buf_insert_size(_to, _from, _pos, (_from)->size)
#define buf_append(_to, _from)						\
	buf_append_size(_to, _from, (_from)->size)
#define buf_prepend_size(_to, _from, _size) 				\
	buf_insert_size(_to, _from, 0, _size)
#define buf_prepend(_to, _from) 					\
	buf_insert_size(_to, _from, 0, (_from)->size)


/* buffer.c */
buffer_t *buf_new_dummy(area_t *);
buffer_t *buf_new(area_t *, void *, size_t);
void _buf_free(buffer_t *);
void buf_realloc(buffer_t **, size_t);
int buf_change_area(buffer_t *, area_t *);
buffer_t *buf_dup(buffer_t *);
buffer_t *buf_dup_size(buffer_t *, size_t);
int buf_append_size(buffer_t **, buffer_t *, size_t);
int buf_copy(buffer_t **, buffer_t *, size_t, size_t, size_t);
int buf_insert_size(buffer_t **, buffer_t *, size_t, size_t);
buffer_t *buf_new_pack(area_t *, char *, ...);
int buf_append_pack(buffer_t **, char *, ...);
int buf_insert_pack(buffer_t **, size_t pos, char *, ...);
size_t buf_unpack_data(area_t *, void *, size_t, char *, ...);
size_t buf_unpack(buffer_t *, char *, ...);

#endif

