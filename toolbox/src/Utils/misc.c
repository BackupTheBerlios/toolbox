//========================================================================
// 	$Id: misc.c,v 1.1 2004/05/12 22:04:53 plg Exp $
//========================================================================
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

/**
 * @file misc.c various logging utilities
 */

/**
 * @defgroup Logger Logging
 * Log related utilities
 */

//#define WITH_SYSLOG

#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "tb_global.h"

#include "Toolbox.h"
#include "Memory.h"
#include "misc.h"
#include "strfmt.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>

#ifdef WITH_SYSLOG
pthread_once_t _openlog_once = PTHREAD_ONCE_INIT;   
static char *getMyName(void);
static void openlog_once(void); 
char _syslog_ident[20];
#endif

void          * _tracefnc     = NULL;
void          * _getnamefnc   = NULL;
TB_LOGLEVELS    tb_errorlevel = DEFAULT_ERRLEVEL;

struct timeval profile_t1, profile_t2, *__p_t1, *__p_t2;
pthread_once_t _profile_once = PTHREAD_ONCE_INIT;   
pthread_once_t _trace_once   = PTHREAD_ONCE_INIT;   
int	_tb_profile_ready = 0;
static double mdiff(	struct timeval t1, struct timeval t2);

char *dbg_errlevels[] = { 
	"F",
	"A",
	"C",
	"E",
	"W",
	"N",
	"I",
	"D"
};


void setup_trace_once(void) {
	char *s;
	if((s = getenv("tb_debug")))  tb_errorlevel = TB_CLAMP((atoi(s)), 0, 7);
}



/** Logger function
 *
 * @ingroup Logger
 */
void tb_trace(int level, char *format, ...) {
  time_t   time_now;
  struct   tm temps_now;
  char     curDate[20];
  int l;
  va_list  ap;
	char     message[257];

	pthread_once(&_trace_once, setup_trace_once);

  if(level > tb_errorlevel) return;
	if(level < TB_EMERG) level = TB_EMERG;

  va_start(ap, format);
	tb_vsnprintf(message, 256, format, ap);
  va_end(ap);

  l = strlen(message);
  if(message[l-1] == '\n') message[l-1] = 0;

#ifdef WITH_SYSLOG
	pthread_once(&_openlog_once, openlog_once);
	syslog(level, "%s %s", dbg_errlevels[level], message);
#endif

	if(_tracefnc != NULL) {
		((void(*)(char*))_tracefnc)(message);
	} else {
		time_now = time(NULL);
		localtime_r(&time_now, &temps_now);
		strftime(curDate, sizeof curDate, "%d/%m %H:%M:%S", &temps_now);
		fprintf(stderr, "%s %s [%d/%ld] %s\n", 
						curDate, 
						dbg_errlevels[level],
						getpid(), 
						pthread_self(), 
						message);
		fflush(stderr);
	}
}

void setup_profile_once(void) {
	_tb_profile_ready = 1;
	gettimeofday(&profile_t1, NULL);
	gettimeofday(&profile_t2, NULL);
	__p_t1 = & profile_t1;
	__p_t2 = & profile_t2;
}


static double mdiff(	struct timeval t1, struct timeval t2) {
	 return (double)(
					 (((double)t2.tv_sec*1000000) + ((double)t2.tv_usec))   - 
					 (((double)t1.tv_sec*1000000) + ((double)t1.tv_usec))) / (double)1000000;
}


/** Profiling function
 * @ingroup Logger
 */
void tb_profile(char *format, ...) {
  time_t   time_now;
  struct   tm temps_now;
  char     curDate[20];
  int l;
  va_list  ap;
	char     message[257];
	struct timeval *swp;

	pthread_once(&_profile_once, setup_profile_once);
	gettimeofday(__p_t2, NULL);

  va_start(ap, format);
	tb_vsnprintf(message, 256, format, ap);
  va_end(ap);

  l = strlen(message);
  if(message[l-1] == '\n') message[l-1] = 0;

	time_now = time(NULL);
	localtime_r(&time_now, &temps_now);
	strftime(curDate, sizeof curDate, "%d/%m %H:%M:%S", &temps_now);

	fprintf(stderr, "%s +%02.06f [%d/%ld] %s\n", 
					curDate, 
					mdiff(*__p_t1, *__p_t2),
					getpid(), 
					pthread_self(), 
					message);
	fflush(stderr);
	swp = __p_t1;
	__p_t1 = __p_t2;
	__p_t2 = swp;
}



#ifdef WITH_SYSLOG
void openlog_once() {
	char *ident = getMyName();
	if(ident == NULL) return;
	snprintf(_syslog_ident, 20, "%s", ident);
	openlog(_syslog_ident, LOG_PID, LOG_LOCAL7);
	tb_xfree(ident);
}
#endif


void tb_setTraceFnc( void *fnc ) {
	_tracefnc = fnc;
}
void tb_setGetNameFnc( char*(*fnc)(void) ) {
	_getnamefnc = fnc;
}



char *getMyName() {
	char *id = NULL;
	char *s;
	if( _getnamefnc == NULL ) {
		FILE * fd;
		char buffer[MAX_BUFFER];
		Vector_t V = tb_Vector();
		if( (fd = fopen("/proc/self/stat", "r")) == NULL ) return NULL;
		if( fgets(buffer, MAX_BUFFER-1, fd) == NULL) { fclose(fd); return NULL; }
		fclose(fd);
		tb_tokenize(V, buffer, " ", 0);
		__tb_asprintf(&id, "%s", (tb_toStr(V, 1))+1);
		if((s=strchr(id, ')')) != NULL) { *s=0; }
		tb_Free(V);
	} else {
		id = (char *)((char*(*)(void))_getnamefnc)();
		fprintf(stderr, "->>>> %s <<<<<<<\n", id);
		return ( id );
	}
	return id;		 
}


Hash_t tb_readConfig(char *file) { // fixme : redo this ; use tb_Properties

	char *buffer, *s;
	Vector_t V;
	FILE *FH;
	Hash_t H, curH;
	String_t S;

	if((FH = fopen(file, "r")) == NULL) {
		tb_trace(TB_CRIT, "tb_readConfig :open <%s> %s\n", file, strerror(errno));
		return NULL;
	}

	H = tb_Hash();
	curH = H;
	S = tb_String(NULL);
	V = tb_Vector();
	
	assert((buffer = tb_xmalloc(MAX_BUFFER)));

	while( (s = fgets(buffer, MAX_BUFFER-1, FH))) {
		tb_StrAdd(tb_Clear(S), 0, "%s", s);
		if(tb_matchRegex(S, "^\\s*[#;]", 0)) continue;
		if(tb_matchRegex(S, "^\\s*$", 0)) continue;
		if(tb_getRegex(tb_Clear(V), S, "^\\s*\\[([^\\]]+)\\]", 
									 PCRE_MATCHMULTI)) { //start section
			
			tb_Replace(H, curH = tb_Hash(), tb_toStr(V,0));

		} 
		if( tb_getRegex(tb_Clear(V), S, "^\\s*([^=\\s]+)\\s*=\\s*(.*)$", PCRE_MATCHMULTI)) {
			
			tb_Replace(curH, tb_Pop(V), tb_toStr(V,0));

		}
	}
	tb_Free(S);
	tb_Free(V);

	fclose(FH);

	return H;
}




