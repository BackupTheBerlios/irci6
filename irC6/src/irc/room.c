
/* $Id: room.c,v 1.1 2005/04/16 09:23:26 benkj Exp $ */

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

#include "irC6.h"
#include "error.h"
#include "str_missing.h"

char *
irc_room_encode(area_t *area, char *room)
{
	char *ret, *p, *q;

	if (*room == '\0')
		return "";

	q = ret = a_malloc(area, (strlen(room) * 3) + 2);
	*q++ = '#';

	for (p = room; *p != '\0'; p++) {
		if (!isalnum(*p)) {
			sprintf(q, "%%%.02X", (unsigned int)*p & 0xff);
			q += 3;
		} else
			*q++ = *p;
	}

	*q = '\0';

	return ret;
}

static unsigned int __hexcode[256] = {
	['a'] = 0x1AA,['A'] = 0x1AA,['b'] = 0x1BB,['B'] = 0x1BB,
	['c'] = 0x1CC,['C'] = 0x1CC,['d'] = 0x1DD,['D'] = 0x1DD,
	['e'] = 0x1EE,['E'] = 0x1EE,['f'] = 0x1FF,['F'] = 0x1FF,
	['0'] = 0x100,['1'] = 0x111,['2'] = 0x122,['3'] = 0x133,
	['4'] = 0x144,['5'] = 0x155,['6'] = 0x166,['7'] = 0x177,
	['8'] = 0x188,['9'] = 0x199
};

#define IS_HEX(_c)	(__hexcode[(unsigned int)(_c) & 0xff] != 0)
#define HEX_1(_c)	(__hexcode[(unsigned int)(_c) & 0xff] & 0xf0)
#define HEX_2(_c)	(__hexcode[(unsigned int)(_c) & 0xff] & 0x0f)

char *
irc_room_decode(area_t *area, char *room)
{
	char *ret, *p, *q;

	ret = a_malloc(area, strlen(room));
	p = (*room == '#') ? (room + 1) : room;

	for (q = ret; *p != '\0'; p++, q++)
		switch (*p) {
		case '%':
			if (IS_HEX(*(p + 1)) && IS_HEX(*(p + 2))) {
				*q = (char)(HEX_1(*(p + 1)) | HEX_2(*(p + 2)));
				p += 2;

				break;
			}
		default:
			*q = *p;
		}

	*q = '\0';

	return ret;
}
