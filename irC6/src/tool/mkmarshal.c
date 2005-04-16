
/* $Id: mkmarshal.c,v 1.1 2005/04/16 09:23:32 benkj Exp $ */

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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define VERSION		"0.1"

static FILE *fin, *fout_c, *fout_h;
static char *outbase = "out.marshal", *name_prefix = "";
static int use_data = 0;
static char *includes[2048];
static unsigned int include_idx = 0;
static char *typefixed[2048];
static unsigned int typefixed_idx = 0;
static char tmp[2048];

static void
__error(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

#define SKIP_SPACES(_p)							\
	while (*_p == ' ' || *_p == '\t') _p++;

static int
is_void(char *p)
{
	if (p == NULL)
		return 0;
	else if (*p == '\0')
		return 1;

	SKIP_SPACES(p);

	if (strncmp(p, "void", 4))
		return 0;
	else
		p += 4;

	SKIP_SPACES(p);

	return (*p != '*');
}

static char *
typefix(char *p)
{
	unsigned int i, usigned = 0;
	char *type = p;

	if (*p == '\0')
		return "int";

	SKIP_SPACES(p);

	if (!strncmp(p, "unsigned", 7)) {
		usigned = 1;
		p += 8;
		SKIP_SPACES(p);
	}

	for (i = 0; i < typefixed_idx; i++)
		if (!strncmp(p, typefixed[i], strlen(typefixed[i])))
			break;

	if (i == typefixed_idx)
		return type;
	else
		p += strlen(typefixed[i]);

	SKIP_SPACES(p);

	return (*p == '*') ? type : (usigned ? "unsigned int" : "int");
}

static void
remove_nl(char *p)
{
	unsigned int i;

	i = strlen(p);
	if (i > 0 && p[i - 1] == '\n')
		p[i - 1] = '\0';
}

static int
parse_line(char *line, char **name, char **ret, char **param)
{
	if (*line == '#' || (*name = strsep(&line, ":")) == NULL)
		return 0;

	*ret = strsep(&line, ":");
	*param = strsep(&line, ":");

	if (is_void(*ret))
		*ret = NULL;
	if (is_void(*param))
		*param = NULL;

	return 1;
}

static void
out_head()
{
	unsigned int i;
	char *p;

	for (i = 0, p = outbase; i < (sizeof(tmp) - 1) && *p != '\0'; p++)
		if (isalnum(*p))
			tmp[i++] = *p;

	tmp[i] = '\0';

	fprintf(fout_h, "#ifndef __marshal_%s\n", tmp);
	fprintf(fout_h, "#define __marshal_%s\n\n", tmp);
	fprintf(fout_h, "#include <sys/types.h>\n");
	fprintf(fout_h, "#include <stdarg.h>\n\n");

	for (i = 0; i < include_idx; i++)
		fprintf(fout_h, "#include <%s>\n", includes[i]);

	fprintf(fout_h, "\n#ifndef __marshal_types\n");
	fprintf(fout_h, "#define __marshal_types\n");
	fprintf(fout_h, "typedef void (*marshal_callback_t)(void);\n\n");
	fprintf(fout_h, "typedef void (*marshal_func_t)(");
	fprintf(fout_h, "marshal_callback_t, va_list, void *);\n");
	fprintf(fout_h, "typedef void (*marshal_func_data_t)(");
	fprintf(fout_h, "marshal_callback_t, va_list, void *, void *);\n");
	fprintf(fout_h, "#endif\n\n");

	fprintf(fout_c, "#include <stdio.h>\n");
	fprintf(fout_c, "#include \"%s.h\"\n\n", outbase);
}

static void
out_func_prototype(char *name)
{
	fprintf(fout_h, "void %s%s(marshal_callback_t, va_list,%s void *);\n",
		name_prefix, name, use_data ? "void *," : "");
}

static void
out_func_body(char *name, char *ret, char *param)
{
	char *p;
	unsigned int i, args;

	fprintf(fout_c, "void %s%s(marshal_callback_t cb, va_list ap,",
		name_prefix, name);
	fprintf(fout_c, "%s void *retv)\n{\n", use_data ? "void *data," : "");

	if (ret != NULL)
		fprintf(fout_c, "\t%s ret;\n", ret);

	snprintf(tmp, sizeof(tmp), "%s%s", (param ? param : ""),
		 (use_data ? (param ? ", void *" : "void *") : ""));

	for (args = 0; (p = strsep(&param, ",")) != NULL; args++)
		fprintf(fout_c, "\t%s arg%u = (%s)va_arg(ap, %s);\n",
			p, args, p, typefix(p));

	fprintf(fout_c, ret ? "\n\tret = " : "\n\t");
	fprintf(fout_c, "((%s (*)(%s))cb)(", ret ? ret : "void", tmp);

	if (args)
		fprintf(fout_c, "arg0");

	for (i = 1; i < args; i++)
		fprintf(fout_c, ", arg%u", i);

	if (use_data)
		fprintf(fout_c, "%sdata", args ? ", " : "");

	fprintf(fout_c, ");\n");

	if (ret) {
		fprintf(fout_c, "\n\tif (retv != NULL)");
		fprintf(fout_c, "\n\t\t*((%s *)retv) = ret;\n", ret);
	}

	fprintf(fout_c, "}\n\n");
}

static void
out_bottom()
{
	fprintf(fout_h, "\n\n#endif\n");
}

static void
mkmarshal()
{
	char line[2048], *name, *ret, *param;

	out_head();

	while (fgets(line, sizeof(line), fin) != NULL &&
	       !feof(fin) && !ferror(fin)) {

		remove_nl(line);

		if (!parse_line(line, &name, &ret, &param))
			continue;

		out_func_prototype(name);
		out_func_body(name, ret, param);
	}

	out_bottom();
}

int
main(int argc, char **argv)
{
	int cc;

	fin = stdin;
	typefixed[0] = "char";
	typefixed[1] = "short";
	typefixed_idx += 2;


	while ((cc = getopt(argc, argv, "o:di:t:p:vh")) != -1)
		switch (cc) {
		case 'o':
			outbase = optarg;
			break;
		case 'd':
			use_data = 1;
			break;
		case 'i':
			assert(include_idx < 2048);
			includes[include_idx++] = optarg;
			break;
		case 't':
			assert(typefixed_idx < 2048);
			typefixed[typefixed_idx++] = optarg;
			break;
		case 'p':
			name_prefix = optarg;
			break;
		case 'v':
			puts(VERSION);
			exit(EXIT_SUCCESS);
		case 'h':
			printf("usage: %s\t[options] [input file]\n\n",
			       argv[0]);
			printf("options:\n");
			printf(" -o base\tbase name for output files\n");
			printf(" -d \t\tpass data argument to function\n");
			printf(" -i header\tfor includin header in source\n");
			printf(" -t type\tsizeof(type) < sizeof(int)\n");
			printf(" -p prefix\tname prefix for marshal func\n");
			exit(EXIT_SUCCESS);
		default:
			printf("use `%s -h' to see help\n", argv[0]);
			exit(EXIT_FAILURE);
		}

	if (optind < argc)
		if ((fin = fopen(argv[optind], "r")) == NULL)
			__error("can't open %s for reading", argv[optind]);


	snprintf(tmp, sizeof(tmp), "%s.c", outbase);
	if ((fout_c = fopen(tmp, "w")) == NULL)
		__error("can't open %s for writing", tmp);

	snprintf(tmp, sizeof(tmp), "%s.h", outbase);
	if ((fout_h = fopen(tmp, "w")) == NULL)
		__error("can't open %s for writing", tmp);

	setlinebuf(fin);

	mkmarshal();

	fclose(fin);
	fclose(fout_c);
	fclose(fout_h);

	return 0;
}
