#! /bin/sh

# $Id: ev_test.sh,v 1.1 2005/04/16 09:23:19 benkj Exp $ */

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

cat > .event_test.c << EOF
#include <sys/types.h>
#include <sys/time.h>
#include <event.h> 

int main() { 
	event_init(); 
	return 0;
}
EOF

$* -o .event_test.bin .event_test.c -levent >/dev/null 2>&1

if ! test -x .event_test.bin; then
	echo "per compilare irC6 dovete installare libevent"
	echo "http://monkey.org/~provos/libevent/"
	echo "per linux consiglio di installare una versione >=0.7.x"
	
	exit 1
fi
exit 0
