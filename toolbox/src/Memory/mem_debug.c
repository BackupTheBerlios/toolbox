//==========================================================
// $Id: mem_debug.c,v 1.1 2004/05/12 22:04:50 plg Exp $
//==========================================================
/* Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
 *
 * This file is part of Toolbox, an object-oriented utility library
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 */

#ifndef __BUILD
#define __BUILD
#endif

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "fastmem.h"

void *dbg_malloc(char *func, char *file, int line, size_t sz) {
	void *mem = malloc(sz);
	_setup_fm();

	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "*M{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, sz, func, file, line);
	}
	return mem;
}

char *dbg_strdup(char* func, char *file, int line, char *str) {
		int len;
	char *dup = malloc(len = strlen(str)+1);
	memcpy(dup, str, len);
	dup[len] = 0;
	_setup_fm();

	if(fm->dbg && fm->dbg->fm_trace_fh && dup) {
		fprintf(fm->dbg->fm_trace_fh, "*S{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), dup, strlen(dup), func, file, line);
			}
	return dup;
}

void *dbg_realloc(char* func, char *file, int line, void *mem, size_t sz) {
	void *new = realloc(mem, sz);
	_setup_fm();

	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "*R{%ld}:%p:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, new, sz, func, file, line);
	}
	return new;
}

void *dbg_calloc(char* func, char *file, int line, size_t nb, size_t sz) {
	void *mem = calloc(sz, nb);
	_setup_fm();

	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "*C{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, sz*nb, func, file, line);
		}
	return mem;
}

void dbg_free(char* func, char *file, int line, void *mem) {
	free(mem);
	_setup_fm();

	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "*F{%ld}:%p:%s:%s:%d\n", 
						pthread_self(), mem, func, file, line);
			}
			return;
}


