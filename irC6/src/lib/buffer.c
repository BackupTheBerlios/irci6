
/* $Id: buffer.c,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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
#include <stdarg.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <ctype.h>

#include <buffer.h>

#define _BUF_DATA(_ptr) 						\
	(void *)((char *)(_ptr) + sizeof(buffer_t))
#define _BUF_DATA_OFFS(_ptr, _offs) 					\
	(void *)((char *)(_ptr) + sizeof(buffer_t) + (_offs))
#define _BUF_SIZE(_size)						\
	(sizeof(buffer_t) + (_size))
#define _BUF_EXPAND(_buf, _size)					\
	a_expand((*(_buf))->area, _buf, _BUF_SIZE(_size))
#define _BUF_ZERO(_buf, _offs, _size)					\
	memset(_BUF_DATA_OFFS(_buf, _offs), 0, _size)
#define _BUF_COPY(_src, _soffs, _dst, _doffs, _size)			\
	memcpy(_BUF_DATA_OFFS(_src, _soffs), 				\
	       _BUF_DATA_OFFS(_dst, _doffs), _size)
#define _BUF_MOVE(_buf, _offs, _doffs, _size)				\
	memmove(_BUF_DATA_OFFS(_buf, (_offs) + (_doffs)),		\
		_BUF_DATA_OFFS(_buf, (_offs)), _size)

buffer_t *
buf_new_dummy(area_t *area)
{
	buffer_t *ret;

	ret = a_alloc(area, _BUF_SIZE(0));

	if (ret != NULL) {
		ret->area = area;
		ret->size = 0;
	}

	return ret;

}

buffer_t *
buf_new(area_t *area, void *data, size_t size)
{
	buffer_t *ret;

	ret = (data != NULL) ? a_alloc(area, _BUF_SIZE(size)) :
	    a_alloc0(area, _BUF_SIZE(size));

	if (ret != NULL) {
		ret->area = area;
		ret->size = size;

		if (data != NULL)
			memcpy(_BUF_DATA(ret), data, size);
	}

	return ret;
}

void
_buf_free(buffer_t *buf)
{
	a_free(buf->area, buf);
}

void
buf_realloc(buffer_t **buf, size_t size)
{
	if (*buf != NULL) {
		*buf = a_realloc0((*buf)->area, *buf, _BUF_SIZE(size));
		if (*buf != NULL)
			(*buf)->size = size;
	}
}

int
buf_change_area(buffer_t *buf, area_t *new)
{
	if (!area_relink(buf->area, new, buf))
		return 0;

	buf->area = new;
	return 1;
}

buffer_t *
buf_dup(buffer_t *buf)
{
	return a_memdup(buf->area, buf, _BUF_SIZE(buf->size));
}

buffer_t *
buf_dup_size(buffer_t *from, size_t size)
{
	buffer_t *to;

	if (from->size < size) {
		to = a_alloc(from->area, _BUF_SIZE(size));
		if (to == NULL)
			return NULL;

		to->area = from->area;
		to->size = size;

		_BUF_COPY(to, 0, from, 0, from->size);
		_BUF_ZERO(to, from->size, size - from->size);
	} else {
		to = a_memdup(from->area, from, _BUF_SIZE(size));
		if (to != NULL)
			to->size = size;
	}

	return to;
}

int
buf_append_size(buffer_t **to, buffer_t *from, size_t size)
{
	if (*to == NULL) {
		*to = buf_dup_size(from, size);
		return (*to != NULL);
	}

	if (_BUF_EXPAND(to, (*to)->size + from->size)) {
		_BUF_COPY(*to, (*to)->size, from, 0, from->size);
		(*to)->size += from->size;

		return 1;
	} else
		return 0;
}

int
buf_copy(buffer_t **to, buffer_t *from, size_t toffs, size_t foffs, size_t size)
{
	if (*to == NULL) {
		*to = buf_new(from->area, NULL, size + toffs);

		if (*to == NULL)
			return 0;
	} else if ((*to)->size < toffs + size) {
		size_t new_size = toffs + size;

		if (!_BUF_EXPAND(to, new_size))
			return 0;

		if (toffs > (*to)->size)
			_BUF_ZERO(*to, (*to)->size, toffs - (*to)->size);

		(*to)->size = new_size;
	}

	if (from->size < foffs + size) {
		if (from->size > foffs)
			_BUF_COPY(*to, toffs, from, foffs, from->size - foffs);

		_BUF_ZERO(*to, toffs + (from->size - foffs), 
		    size - (from->size - foffs));
	} else
		_BUF_COPY(*to, toffs, from, foffs, size);

	return 1;
}

int
buf_insert_size(buffer_t **to, buffer_t *from, size_t pos, size_t size)
{
	size_t old_s;

	if (*to == NULL)
		return buf_copy(to, from, pos, 0, size);

	old_s = (*to)->size;
	(*to)->size = MAX(pos, old_s) + size;

	if (_BUF_EXPAND(to, (*to)->size)) {
		if (old_s > pos)
			_BUF_MOVE(*to, pos, size, old_s - pos);
		_BUF_COPY(*to, pos, from, 0, MIN(from->size, size));

		if (pos > old_s)
			_BUF_ZERO(*to, old_s, pos - old_s);
		if (size > from->size)
			_BUF_ZERO(*to, pos + from->size, size - from->size);

		return 1;
	} else
		return 0;
}

SIMPLEQ_HEAD(queue_head, queue_node);

struct queue_node {
	void *ptr;
	size_t size;
	       SIMPLEQ_ENTRY(queue_node) queue;
};

#define PARAM_RESET(_q)							\
do {									\
	memset(&_q, 0, sizeof(struct queue_node));			\
	bigendian = 0;							\
} while (0)

#define QUEUE_ADD(_qh, _qq, _size)					\
do {									\
	struct queue_node *_q;						\
	if (_qq.ptr != NULL && _qq.size > 0) {				\
		_q = a_memdup(area, &_qq, sizeof(struct queue_node));	\
		if (_q == NULL)						\
			goto error;					\
		SIMPLEQ_INSERT_TAIL(_qh, _q, queue);			\
		_size += _qq.size;					\
	}								\
	memset(&_qq, 0, sizeof(struct queue_node));			\
	bigendian = 0;							\
} while(0)

#define _GET_ARG(___ptr, ___ap, ___type, ___dtype, ___bigf)		\
	*(___type *)___ptr = ___bigf((___type)va_arg(___ap,  ___dtype))

#define _GET_ARG_BE(__ptr, __ap, __type, __dtype, __bigf)		\
	if (bigendian) {						\
		_GET_ARG(__ptr, __ap, __type, __dtype, __bigf);		\
	} else {							\
		_GET_ARG(__ptr, __ap, __type, __dtype, );		\
	}

#define BUILD_NUM_PTR(_ptr, _ap, _type, _dtype, _bigf)			\
do {									\
	if (usigned) {							\
		_GET_ARG_BE(_ptr, _ap, unsigned _type, 			\
		    	    unsigned _dtype, _bigf)			\
	} else {							\
		_GET_ARG_BE(_ptr, _ap, _type, _dtype, _bigf)		\
	}								\
} while (0)

#define ADD_CHAR(_fmt)							\
do {									\
	percent = 0;							\
	bigendian = 0;							\
									\
	q.size = 1;							\
	q.ptr = _fmt;							\
	QUEUE_ADD(qh, q, totsize);					\
} while (0)

#define SIZE_CHAR	sizeof(char)
#define SIZE_SHORT	sizeof(short)
#define SIZE_INT	sizeof(int)
#define SIZE_LONG	sizeof(long int)
#define SIZE_QUAD	sizeof(quad_t)	/* XXX: not supported */

static size_t
_build_pack_queue(area_t *area, struct queue_head *qh, char *fmt, va_list ap)
{
	buffer_t *r;
	struct queue_node q;
	int percent, usigned, bigendian;
	size_t totsize;

	SIMPLEQ_INIT(qh);
	memset(&q, 0, sizeof(struct queue_node));
	percent = 0;
	totsize = 0;
	bigendian = 0;

	for (; *fmt != '\0'; fmt++) {
		if (*fmt == '%') {
			if (percent)
				ADD_CHAR(fmt);
			else
				percent = 1;
			continue;
		}

		if (!percent) {
			ADD_CHAR(fmt);
			continue;
		}

		switch (*fmt) {
		case 'p':
			percent = 0;

			q.ptr = va_arg(ap, void *);

			QUEUE_ADD(qh, q, totsize);
			continue;
		case 's':
			percent = 0;

			q.ptr = va_arg(ap, char *);

			q.size = strlen((char *)q.ptr);

			QUEUE_ADD(qh, q, totsize);
			continue;
		case 'r':
			percent = 0;

			r = va_arg(ap, buffer_t *);

			if (r != NULL) {
				q.ptr = _BUF_DATA(r);
				if (q.size == 0 || q.size > r->size)
					q.size = r->size;
			} else
				q.size = 0;

			QUEUE_ADD(qh, q, totsize);
			continue;
		case 'u':
		case 'i':
			percent = 0;

			usigned = (*fmt == 'u');

			if (q.size == 0)
				q.size = SIZE_INT;
			q.ptr = a_alloc0(area, q.size);
			if (q.ptr == NULL) {
				PARAM_RESET(q);
				continue;
			}

			switch (q.size) {
#ifdef SIZEOFLONG_DIF_SIZEOFINT
			case SIZE_LONG:
				BUILD_NUM_PTR(q.ptr, ap, long int, long int,
					      htonl);

				break;
#endif
			case SIZE_INT:
				BUILD_NUM_PTR(q.ptr, ap, int, int, htonl);

				break;
			case SIZE_SHORT:
				BUILD_NUM_PTR(q.ptr, ap, short, int, htons);

				break;
			case SIZE_CHAR:
				BUILD_NUM_PTR(q.ptr, ap, char, int,);

				break;
			default:
				goto error;
			}

			QUEUE_ADD(qh, q, totsize);

			continue;
		case 'Z':
			ADD_CHAR("");

			continue;
		}

		if (q.size != 0)
			goto error;

		switch (*fmt) {
		case '^':
			bigendian = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			do {
				q.size = (q.size * 10) + (*fmt++ - '0');
			} while (isdigit(*fmt));

			fmt--;
			continue;
		case 'l':
			q.size = SIZE_LONG;
			continue;
		case 'h':
			q.size = SIZE_SHORT;
			continue;
		case 'b':
			q.size = SIZE_CHAR;
			continue;
		default:
			ADD_CHAR(fmt);
		}
	}

	return totsize;

      error:
	return 0;
}

static void
_dequeue_pack(struct queue_head *head, void *_data)
{
	register struct queue_node *q;
	register char *data = _data;

	SIMPLEQ_FOREACH(q, head, queue) {
		memcpy(data, q->ptr, q->size);
		data += q->size;
	}
}

buffer_t *
buf_new_pack(area_t *area, char *fmt, ...)
{
	buffer_t *ret = NULL;
	area_t *atmp;
	va_list ap;
	size_t size;
	struct queue_head head;

	atmp = area_node(area);
	if (atmp == NULL)
		return NULL;

	va_start(ap, fmt);
	size = _build_pack_queue(atmp, &head, fmt, ap);
	va_end(ap);

	if (size > 0) {
		ret = a_alloc(area, _BUF_SIZE(size));

		if (ret != NULL) {
			ret->area = area;
			ret->size = size;

			_dequeue_pack(&head, _BUF_DATA(ret));
		}
	}

	area_destroy(atmp);

	return ret;
}

int
buf_append_pack(buffer_t **buf, char *fmt, ...)
{
	area_t *atmp;
	va_list ap;
	size_t size;
	struct queue_head head;

	atmp = area_node((*buf)->area);
	if (atmp == NULL)
		return 0;

	va_start(ap, fmt);
	size = _build_pack_queue(atmp, &head, fmt, ap);
	va_end(ap);

	if (size == 0) {
		area_destroy(atmp);
		return 0;
	}

	if (_BUF_EXPAND(buf, (*buf)->size + size)) {
		_dequeue_pack(&head, _BUF_DATA_OFFS(*buf, (*buf)->size));

		(*buf)->size += size;
	}

	area_destroy(atmp);

	return (*buf != NULL);
}

int
buf_insert_pack(buffer_t **buf, size_t pos, char *fmt, ...)
{
	area_t *atmp;
	va_list ap;
	size_t size, old_s, new_s;
	struct queue_head head;

	atmp = area_node((*buf)->area);
	if (atmp == NULL)
		return 0;

	va_start(ap, fmt);
	size = _build_pack_queue(atmp, &head, fmt, ap);
	va_end(ap);

	if (size == 0) {
		area_destroy(atmp);
		return 0;
	}

	old_s = *buf ? (*buf)->size : 0;
	new_s = MAX(pos, old_s) + size;

	if (_BUF_EXPAND(buf, new_s)) {
		(*buf)->size = new_s;

		if (old_s > pos)
			_BUF_MOVE(*buf, pos, size, old_s - pos);
		else if (pos > old_s)
			_BUF_ZERO(*buf, old_s, pos - old_s);
		
		_dequeue_pack(&head, _BUF_DATA_OFFS(*buf, pos));
	}
	
	area_destroy(atmp);
	
	return (*buf != NULL);
}


SIMPLEQ_HEAD(sizel_head, sizel_entry);

struct sizel_entry {
	size_t size;
	       SIMPLEQ_ENTRY(sizel_entry) queue;
};

#define RESET_VAR()							\
do {									\
	percent = 0;							\
	pos = 0;							\
	size = 0;							\
	position = 0;							\
	bigendian = 0;							\
} while (0)

#define CHECK_CHAR(_ch)							\
do {									\
	if (data[offs++] != _ch)					\
		goto error;						\
	RESET_VAR();							\
} while (0)

#define _SET_ARG(___ptr, ___from, ___type, ___bigf)			\
	*(___type *)___ptr = ___bigf(*((___type *)(___from)))

#define _SET_ARG_BE(__ptr, __from, __type, __bigf)			\
	if (bigendian) {						\
		_SET_ARG(__ptr, __from, __type, __bigf);		\
	} else {							\
		_SET_ARG(__ptr, __from, __type, );			\
	}

#define PARSE_NUM_PTR(_ptr, _from, _type, _bigf)			\
do {									\
	if (usigned) {							\
		_SET_ARG_BE(_ptr, _from, unsigned _type, _bigf)		\
	} else {							\
		_SET_ARG_BE(_ptr, _from, _type, _bigf)			\
	}								\
} while (0)


static size_t
_unpack_data(area_t *area, char *data, size_t data_len, char *fmt, va_list ap)
{
	area_t *atmp;
	size_t size, offs, i, position;
	int percent, usigned, pos, bigendian;
	struct sizel_head head;
	struct sizel_entry *sl;

	atmp = area_node(area);
	if (atmp == NULL)
		return 0;

	SIMPLEQ_INIT(&head);
	RESET_VAR();

	for (offs = 0; *fmt != '\0'; fmt++) {
		if (offs + size > data_len)
			goto error;

		if (*fmt == '%') {
			if (percent) {
				CHECK_CHAR(*fmt);
				continue;
			}
			percent = 1;

			sl = SIMPLEQ_FIRST(&head);

			if (sl != NULL) {
				size = sl->size;
				SIMPLEQ_REMOVE_HEAD(&head, sl, queue);
			}

			continue;
		}

		if (!percent) {
			CHECK_CHAR(*fmt);
			continue;
		}

		switch (*fmt) {
		case 'r':{
			buffer_t **rptr;

			rptr = va_arg(ap, buffer_t **);

			*rptr = buf_new(area, data + offs, size);
			if (*rptr == NULL)
				goto error;

			offs += size;
			RESET_VAR();
			continue;
		}
		case 'p':{
			void **vptr;

			vptr = va_arg(ap, void **);

			*vptr = a_alloc(area, size);
			if (*vptr == NULL)
				goto error;

			memcpy(*vptr, data + offs, size);

			offs += size;
			RESET_VAR();
			continue;
		}
		case 's':{
			char **cptr;

			cptr = va_arg(ap, char **);

			if (size == 0) {
				*cptr = a_strdup(area, data + offs);
				if (*cptr == NULL)
					goto error;
				offs += strlen(*cptr) + 1;
			} else {
				*cptr = a_alloc(area, size + 1);
				if (*cptr == NULL)
					goto error;
				memcpy(*cptr, data + offs, size);
				(*cptr)[size] = '\0';

				offs += size;
			}

			RESET_VAR();
			continue;
		}
		case 'u':
		case 'i':{
			void *nptr;

			usigned = (*fmt == 'u' || *fmt == 'U');

			if (size == 0)
				size = SIZE_INT;
			nptr = va_arg(ap, void *);

			switch (size) {
#ifdef SIZEOFLONG_DIF_SIZEOFINT
			case SIZE_LONG:
				PARSE_NUM_PTR(nptr,
					      data + offs, long int, htonl);
				break;
#endif
			case SIZE_INT:
				PARSE_NUM_PTR(nptr, data + offs, int, htonl);

				break;
			case SIZE_SHORT:
				PARSE_NUM_PTR(nptr, data + offs, short, htons);

				break;
			case SIZE_CHAR:
				PARSE_NUM_PTR(nptr, data + offs, char,);

				break;
			default:
				goto error;
			}

			offs += size;

			RESET_VAR();
			continue;
		}
		case '$':{
			unsigned int inum;
			unsigned short snum;
			unsigned char cnum;
			struct sizel_entry *slt;

			usigned = 1;

			sl = a_new(atmp, struct sizel_entry);

			if (sl == NULL)
				goto error;

			if (size == 0)
				size = SIZE_INT;

			switch (size) {
			case SIZE_INT:
				PARSE_NUM_PTR(&inum, data + offs, int, htonl);

				sl->size = (size_t)inum;
				break;
			case SIZE_SHORT:
				PARSE_NUM_PTR(&snum, data + offs, short, htons);

				sl->size = (size_t)snum;
				break;
			case SIZE_CHAR:
				PARSE_NUM_PTR(&cnum, data + offs, char,);

				sl->size = (size_t)cnum;
				break;
			default:
				goto error;
			}

			offs += size;

			if (position == 0) {
				SIMPLEQ_INSERT_HEAD(&head, sl, queue);
				RESET_VAR();
				continue;
			}

			SIMPLEQ_FOREACH(slt, &head, queue) {
				if (--position == 0) {
					SIMPLEQ_INSERT_AFTER(&head,
							     slt, sl, queue);
					RESET_VAR();
					continue;
				}
			}

			for (i = 0; i < position; i++) {
				slt = a_new(atmp, struct sizel_entry);

				if (slt == NULL)
					goto error;

				slt->size = 0;

				SIMPLEQ_INSERT_TAIL(&head, slt, queue);
			}

			SIMPLEQ_INSERT_TAIL(&head, sl, queue);

			RESET_VAR();
			continue;
		}
		case 'k':
			offs += size;

			RESET_VAR();
			continue;
		case 'Z':
			if (data[offs++] != '\0')
				goto error;

			RESET_VAR();
			continue;
		}

		if (size != 0)
			goto error;

		switch (*fmt) {
		case '^':
			bigendian = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':{
			char *p;

			do {
				size = (size * 10) + (*fmt++ - '0');
			} while (isdigit(*fmt));

			if (*fmt == '\0')
				goto error;

			p = fmt--;

			if (*p == 'l' || *p == 'h' || *p == 'b')
				p++;

			if (*p == '$') {
				position = size;
				size = 0;
			}

			break;
		}
		case 'l':
			size = SIZE_LONG;
			break;
		case 'h':
			size = SIZE_SHORT;
			break;
		case 'b':
			size = SIZE_CHAR;
			break;
		default:
			CHECK_CHAR(*fmt);
		}
	}

	area_destroy(atmp);

	return offs;

      error:
	area_destroy(atmp);

	return 0;
}

size_t
buf_unpack_data(area_t *area, void *data, size_t data_len, char *fmt, ...)
{
	va_list ap;
	size_t ret;

	va_start(ap, fmt);
	ret = _unpack_data(area, (char *)data, data_len, fmt, ap);
	va_end(ap);

	return ret;
}

size_t
buf_unpack(buffer_t *buf, char *fmt, ...)
{
	va_list ap;
	size_t ret;

	va_start(ap, fmt);
	ret = _unpack_data(buf->area, (char *)_BUF_DATA(buf), buf->size,
			   fmt, ap);
	va_end(ap);

	return ret;
}
