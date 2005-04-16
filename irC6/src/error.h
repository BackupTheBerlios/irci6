
/* $Id: error.h,v 1.1 2005/04/16 09:23:21 benkj Exp $ */

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

#include <errno.h>

#ifdef IRC6_DEBUG
#define __ERROR_BUILD_STR(__buf, __fmt, __args...)			\
do {									\
	size_t __offs = 0;						\
	    								\
	if (!is_daemon)	 {						\
		extern char *__progname;				\
		snprintf(__buf, sizeof(__buf), "%s: ", __progname);	\
		__offs = strlen(__buf);					\
	} 								\
	    								\
	snprintf(__buf + __offs, sizeof(__buf) - __offs, 		\
	    "%s[%d]: " __fmt , __FILE__ , __LINE__ , ##__args);		\
} while (0)
#else
#define __ERROR_BUILD_STR(__buf, __fmt, __args...)			\
do {									\
	size_t __offs = 0;						\
	    								\
	if (!is_daemon)	 {						\
		extern char *__progname;				\
		snprintf(__buf, sizeof(__buf), "%s: ", __progname);	\
		__offs = strlen(__buf);					\
	} 								\
	    								\
	snprintf(__buf + __offs, sizeof(__buf) - __offs, 		\
	    __fmt , ##__args);						\
} while (0)
#endif

#define FATAL(_fmt, _args...) 						\
do {									\
	extern int is_daemon;						\
	char _buf[1024]; 						\
	__ERROR_BUILD_STR(_buf, _fmt , ##_args);			\
	if (is_daemon)							\
		syslog(LOG_ERR, "%s", _buf);				\
	else								\
		fprintf(stderr, "%s\n", _buf);				\
	exit(EXIT_FAILURE);						\
} while (0)

#define EFATAL(_fmt, _args...) 						\
do {									\
	extern int is_daemon;						\
	int __saved_errno = errno;					\
	char _buf[1024]; 						\
	if (__saved_errno && strerror(__saved_errno)) {			\
		__ERROR_BUILD_STR(_buf, _fmt , ##_args);		\
		if (is_daemon)						\
			syslog(LOG_ERR, "%s: %m", _buf);		\
		else							\
			fprintf(stderr, "%s: %s\n", _buf, 		\
			    strerror(__saved_errno));			\
		exit(EXIT_FAILURE);					\
	} else 								\
		FATAL(_fmt , ##_args); 					\
} while (0)
