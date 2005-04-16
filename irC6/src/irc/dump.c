
/* $Id: dump.c,v 1.1 2005/04/16 09:23:24 benkj Exp $ */

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
#include "irc/irc.h"
#include "error.h"

void
irc_dump_data(connection_t *c, void *ptr, size_t size)
{
	size_t i;

	fprintf(IRC(c)->fout, ":%s NOTICE %s :DUMP ", SERV, c->nick);

	for (i = 0; i < size; i++) {
		if (((i + 1) % ((80 - 8) / 3)) == 0) {
			fprintf(IRC(c)->fout, "\r\n");
			fprintf(IRC(c)->fout, ":%s NOTICE %s :DUMP ",
				SERV, c->nick);
		}

		fprintf(IRC(c)->fout, " %.02x",
			((unsigned int)((char *)(ptr))[i]) & 0xff);
	}

	fprintf(IRC(c)->fout, "\r\n");
	fprintf(IRC(c)->fout, ":%s NOTICE %s :DUMP ", SERV, c->nick);

	for (i = 0; i < size; i++) {
		if (((i + 1) % (80 - 8)) == 0) {
			fprintf(IRC(c)->fout, "\r\n");
			fprintf(IRC(c)->fout, ":%s NOTICE %s :DUMP ",
				SERV, c->nick);
		}

		fprintf(IRC(c)->fout, "%c", isprint(((char *)(ptr))[i]) ?
			((char *)(ptr))[i] : '*');
	}

	fprintf(IRC(c)->fout, "\r\n");
	fflush(IRC(c)->fout);
}
