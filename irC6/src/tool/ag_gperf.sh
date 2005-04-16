#! /bin/sh

DEST=`awk -v arg=$1 '					\
BEGIN{ 							\
	split(arg, vec, /\//);				\
	if (vec[1] == "autogen")			\
		print arg;				\
	else						\
		print "autogen/" arg;			\
}'`
FUNC=`awk -v arg=$1 '					\
BEGIN{ 							\
	split(arg, vec, /\//);				\
	if (vec[1] == "autogen")			\
		print vec[2];				\
	else						\
		print vec[1];				\
}'`

FUNC="${FUNC}_`basename $1`"

gperf 	-t -L KR-C -N $FUNC -D -G -C --ignore-case -W wordlist \
	-o $1.gperf > ${DEST}.c

