//==================================================
// $Id: thread_util.c,v 1.1 2004/05/12 22:04:50 plg Exp $
//==================================================
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

#include <pthread.h>
#include <stdlib.h>
#include "Toolbox.h"
#include "Memory.h"

		

static pthread_key_t  tb_mdbg_key;
static void tb_mdbg_destroy      (void *buf);
static void tb_mdbg_alloc        (void);
static void tb_mdbg_key_create   (void); 

static pthread_once_t tb_mdbg_key_once = PTHREAD_ONCE_INIT;   

/* Allocate the thread-specific buffer */
void set_tb_mdbg(char *func, char *file, int line) {
	mdbg_t Mdbg = NULL;
	pthread_once(&tb_mdbg_key_once, tb_mdbg_key_create);
	Mdbg = pthread_getspecific(tb_mdbg_key);
	if(Mdbg == NULL) tb_mdbg_alloc();
	
	Mdbg = (mdbg_t)pthread_getspecific(tb_mdbg_key);
	Mdbg->func = func;
	Mdbg->file = file;
	Mdbg->line = line;
}
 
/* Return the thread-specific buffer */
mdbg_t get_tb_mdbg() {
	mdbg_t Mdbg = NULL;
	pthread_once(&tb_mdbg_key_once, tb_mdbg_key_create);
	Mdbg = pthread_getspecific(tb_mdbg_key);
	if(Mdbg == NULL) tb_mdbg_alloc();
	
	return (mdbg_t) pthread_getspecific(tb_mdbg_key);
}
 
/* Allocate the key */
static void tb_mdbg_alloc() {
	int rc;
	rc = pthread_setspecific(tb_mdbg_key, tb_xcalloc(1,sizeof(struct mdbg)));
}
 
/* Free the thread-specific buffer */
static void tb_mdbg_destroy(void *buf) {
	tb_xfree(buf);                           
}
	
static void tb_mdbg_key_create() {
	pthread_key_create(&tb_mdbg_key, tb_mdbg_destroy);
}








