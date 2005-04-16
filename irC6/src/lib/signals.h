
/* $Id: signals.h,v 1.1 2005/04/16 09:23:29 benkj Exp $ */

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


#ifndef _SIGNALS_H
#define _SIGNALS_H

#include <sys/types.h>
#include <stdarg.h>
#include <area.h>

#ifndef __marshal_types
#define __marshal_types
typedef void (*marshal_callback_t)(void);

typedef void (*marshal_func_t)(marshal_callback_t, va_list, void *);
typedef void (*marshal_func_data_t)(marshal_callback_t, va_list, void *, void *);
#endif

typedef struct signals_instance siginst_t;

#define SIGNALS_SHARE_TABLE	0x1

#define SIGNALS_LAST	-1U
#define SIGNALS_FIRST	0

#define SIGNALS_FOREVER	-1U

#define SIGNALS_LOCK	0x1

#define sig_connect(_si, _sig, _cb)					\
	_sig_connect(_si, _sig, SIGNALS_LAST, SIGNALS_FOREVER, 		\
		     (marshal_callback_t)_cb)

#define sig_connect_id(_si, _sig, _cb)					\
	_sig_connect_id(_si, _sig, SIGNALS_LAST, SIGNALS_FOREVER, 	\
			(marshal_callback_t)_cb)

#define sig_connect_once(_si, _sig, _cb)				\
	_sig_connect(_si, _sig, SIGNALS_LAST, 1, 			\
		     (marshal_callback_t)_cb)

#define sig_connect_once_id(_si, _sig, _prio, _count, _cb)		\
	_sig_connect_id(_si, _sig, SIGNALS_LAST, 1, 			\
			(marshal_callback_t)_cb)

#define sig_connect_first(_si, _sig, _cb)				\
	_sig_connect(_si, _sig, SIGNALS_FIRST, 1, 			\
		     (marshal_callback_t)_cb)

#define sig_connect_first_id(_si, _sig, _prio, _count, _cb)		\
	_sig_connect_id(_si, _sig, SIGNALS_FIRST, 1, 			\
			(marshal_callback_t)_cb)

#define sig_connect_opt(_si, _sig, _prio, _count, _cb)			\
	_sig_connect(_si, _sig, _prio, _count, (marshal_callback_t)_cb)
#define sig_connect_opt_id(_si, _sig, _prio, _count, _cb)		\
	_sig_connect_id(_si, _sig, _prio, _count, (marshal_callback_t)_cb)
#define sig_disconnect(_si, _sig, _cb)					\
	_sig_disconnect(_si, _sig, (marshal_callback_t)_cb)
#define sig_disconnect_id(_si, _sig, _cb)				\
	_sig_disconnect_id(_si, _sig, (marshal_callback_t)_cb)

#define sig_lock(si, sig)						\
    sig_set_flags(si, sig, SIGNALS_LOCK)
#define sig_lock_id(si, sig)						\
    sig_set_flags_id(si, sig, SIGNALS_LOCK)
#define sig_unlock(si, sig)						\
    sig_unset_flags(si, sig, SIGNALS_LOCK)
#define sig_unlock_id(si, sig)						\
    sig_unset_flags_id(si, sig, SIGNALS_LOCK)
#define sig_is_locked(si, sig)						\
    sig_isset_flags(si, sig, SIGNALS_LOCK)
#define sig_is_locked_id(si, sig)					\
    sig_isset_flags_id(si, sig, SIGNALS_LOCK)

/* lib/signals.c */
siginst_t *sig_instance_new(area_t *);
siginst_t *sig_instance_node(area_t *, siginst_t *);
void sig_instance_quit(siginst_t *);
void sig_instance_destroy(siginst_t *);
unsigned int sig_instance_emitting(siginst_t *);
unsigned int sig_get_id(siginst_t *, const char *);
unsigned int sig_register(siginst_t *, const char *, marshal_func_t);
int sig_unregister(siginst_t *, const char *);
unsigned int _sig_connect(siginst_t *, const char *, unsigned int, unsigned int, marshal_callback_t);
unsigned int _sig_connect_id(siginst_t *, unsigned int, unsigned int, unsigned int, marshal_callback_t);
unsigned int _sig_disconnect(siginst_t *, const char *, marshal_callback_t);
unsigned int _sig_disconnect_id(siginst_t *, const char *, marshal_callback_t);
unsigned int sig_set_flags(siginst_t *, const char *, unsigned int);
unsigned int sig_set_flags_id(siginst_t *, unsigned int, unsigned int);
unsigned int sig_unset_flags(siginst_t *, const char *, unsigned int);
unsigned int sig_unset_flags_id(siginst_t *, unsigned int, unsigned int);
unsigned int sig_isset_flags(siginst_t *, const char *, unsigned int);
unsigned int sig_isset_flags_id(siginst_t *, unsigned int, unsigned int);
void sig_emit(siginst_t *, const char *, ...);
void sig_vemit(siginst_t *, const char *, va_list);
void sig_emit_id(siginst_t *, unsigned int, ...);
void sig_vemit_id(siginst_t *, unsigned int, va_list);

#endif
