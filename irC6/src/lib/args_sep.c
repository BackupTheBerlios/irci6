
/* $Id: args_sep.c,v 1.1 2005/04/16 09:23:28 benkj Exp $ */

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
#include <args_sep.h>

int
args_sep(area_t *area, char *str, int *argc, char ***argv)
{
	char *p, *q;
	int len, k;

	if (argc == NULL || str == NULL)
		return 0;
	
	for (*argc = 0, p = str, q = NULL; *p != '\0'; p++) 
		switch (*p) {
		case ' ':
		case '\n':
		case '\t':
		case '\r':
			q = NULL;
		case '\0':
			break;
		default:
			if (q == NULL)
				(*argc)++;
			q = p;
		}
	

	if (argv == NULL)
		return 1;

	len = strlen(str) + 1;
	k = (*argc + 1) * sizeof(char **);

	if ((q = a_alloc(area,  k + len)) == NULL)
		goto error;

	*argv = (char **)q;
	p = q + k;
	memcpy(p, str, len);

	for (k = 0; k < *argc; ) {
		(*argv)[k] = strsep(&p, " \t\r\n");
		
		if ((*argv)[k] == NULL)
			break;
		if ((*argv)[k][0] != '\0')
			k++;
	}

	if (*argc != k)
		goto error;

	(*argv)[*argc] = NULL;

	return 1;

      error:
	if (argv != NULL && *argv != NULL)
		a_free(area, *argv);
	*argc = 0;

	return 0;
}

char *
args_join(area_t *area, int argc, char **argv)
{
	char *str;
	int i;
	size_t totlen = 0;

	for (i = 0; i < argc; i++)
		totlen += strlen(argv[i]) + 1;

	str = a_alloc(area, totlen);

	if (str != NULL) {
		char *p = str;
		unsigned int len;

		for (i = 0; i < argc; i++) {
			len = strlen(argv[i]);

			memcpy(p, argv[i], len);
			p[len] = ' ';
			p += len + 1;
		}

		str[totlen - 1] = '\0';
	}

	return str;
}
