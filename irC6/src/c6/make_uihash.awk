
# $Id: make_uihash.awk,v 1.1 2005/04/16 09:23:22 benkj Exp $ */

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

# crea src/search/searchui.gperf partendo da 
# src/c6/profilo.ini e src/c6/profilo_nomi.ini

BEGIN {
	FS="[=\r]"

	id_profilo = 0
}

function aggiungi(name, id, val) {
	if (!($2 in profili)) {
		profili[$2] = $2
		num_elm[$2] = 0;
	}

	id_profili[$2, num_elm[$2]] = id
	val_profili[$2, num_elm[$2]] = val

	num_elm[$2]++
}

/^\[Profilo/ { 
	sub("[[]Profilo-0*", "", $1)
	sub("[]]", "", $1)
	id_profilo = int($1)
	next
}

{ if ($2 == "") next; }
/^Valore/ { next; }

/^NomeValore/ { 
#	gsub(/ +$/, "", $2)
#	gsub(/ /, "_", $2)
	sub("NomeValore", "", $1); 
	aggiungi($2, $1, -1);
	next
}

/^[0-9]*/ { 
#	gsub(/ +$/, "", $2)
#	gsub(/ /, "_", $2)
	aggiungi($2, id_profilo, $1);
	next
}

END {
	print "/*\n\tfile generato automaticamente partendo da "
	print "\tsrc/c6/profilo.ini. "
	print "\tNON MODIFICARE, ogni modifica verra' cancellata "
	print "\talla prossima compilazione."
	print "*/\n"
	print "#include \"irC6.h\""
	print "#include \"c6/c6.h\""
	print "#include \"error.h\"\n"

	for (prf in profili) {
		prfz = prf
#		gsub(/-/, "_", prfz)
		gsub(/[^_a-zA-Z0-9]/, "", prfz)

		printf("static const struct values _values_%s[%i] = {", 
			prfz, num_elm[prf])

		for (i = 0; i < num_elm[prf]; i++) {
			if (i != 0)
				printf(", ");
			
			printf("{%i, %i}", id_profili[prf, i], 
				val_profili[prf, i])
		}

		printf("};\n")
	}
	
	print "%}\n"
	print "struct searchui\n\n%%"

	for (prf in profili) {
		prfz = prf
#		gsub(/-/, "_", prfz)
		gsub(/[^_a-zA-Z0-9]/, "", prfz)

		printf("\"%s\", %i, _values_%s\n", prf, num_elm[prf], prfz)
	}
		
	print "%%\n"
}
