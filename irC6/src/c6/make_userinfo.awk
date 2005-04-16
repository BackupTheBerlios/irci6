
# $Id: make_userinfo.awk,v 1.1 2005/04/16 09:23:22 benkj Exp $ */

#
# - irC6 -
#
# Copyright (c) 2003
#	Leonardo Banchi		<benkj@antifork.org>.  
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 
# 
#

# crea c6/userinfo.c partendo da 
# src/c6/profilo.ini e src/c6/profilo_nomi.ini

BEGIN {
	FS="[=\r]"

	print "/*\n\tfile generato automaticamente partendo da "
	print "\tsrc/c6/profilo.ini. "
	print "\tNON MODIFICARE, ogni modifica verra' cancellata "
	print "\talla prossima compilazione."
	print "*/"
	print "\n#include \"irC6.h\"\n#include \"c6/c6.h\"\n\n"

	totale_valori = 256
	num_profilo = 0

	for (i = 0; i < 256; i++)
		valori[i] = "NULL"
}

/^;/ { next; }

/\[NumValori\]/ { next; }

/^\[NomiProfilo\]/ { next; }

/^\[Profilo/ { 
	sub("[[]Profilo-0*", "", $1)
	sub("[]]", "", $1)
	num_profilo = int($1) * 256
	next
}

{ if ($2 == "") next; }

/^TotaleValori/ { totale_valori = int($2); next; }

/^Valore/ { sub("Valore", "", $1); valori[int($1)] = int($2); next;}

/^NomeValore/ { sub("NomeValore", "", $1); nome_valori[int($1)] = $2; next; }

/^[0-9]*/ { profili[num_profilo + int($1)] = $2; next;}

END {
	for (i = 0; i <= totale_valori; i++) {
		printf("static const char *__userinfos%i[%i] = {\n", i, 
			valori[i] + 1);

		for (j = 0; j < valori[i]; j++)
			printf("\"%s\", ", profili[i * 256 + j])
			
		printf("\"%s\"\n};\n", profili[i * 256 + valori[i]])
	}

	printf("\nconst struct userinfos c6_userinfo[%i] = {\n", 
		totale_valori + 1);
	
	for (i = 0; i < totale_valori; i++)
		printf("{\"%s\", %i, __userinfos%i}, ", nome_valori[i], 
			valori[i] + 1, i)
		
	printf("{\"%s\", %i, __userinfos%i}\n}; ", nome_valori[totale_valori],
		valori[totale_valori] + 1, totale_valori)

	printf("\nconst size_t c6_userinfo_elms = %i;\n", totale_valori + 1);
}
