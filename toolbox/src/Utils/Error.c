//==================================================
// $Id: Error.c,v 1.2 2005/05/12 21:52:52 plg Exp $
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

static pthread_key_t  tb_errno_key;
static void tb_errno_destroy      (void *buf);
static void tb_errno_alloc        (void);
static void tb_errno_key_create   (void); 

static pthread_once_t tb_errno_key_once = PTHREAD_ONCE_INIT;   

/* Allocate the thread-specific buffer */
void set_tb_errno(int e) {
	void *key = NULL;
	pthread_once(&tb_errno_key_once, tb_errno_key_create);
	key = pthread_getspecific(tb_errno_key);
	if(key == NULL) tb_errno_alloc();
	
	*(int*)pthread_getspecific(tb_errno_key) = e;
}
 
/* Return the thread-specific buffer */
int get_tb_errno() {
	void *key = NULL;
	pthread_once(&tb_errno_key_once, tb_errno_key_create);
	key = pthread_getspecific(tb_errno_key);
	if(key == NULL) tb_errno_alloc();
	
	return *(int *) pthread_getspecific(tb_errno_key);
}
 
/* Allocate the key */
static void tb_errno_alloc() {
	int rc;
	//	pthread_once(&tb_errno_key_once, tb_errno_key_create);
	rc = pthread_setspecific(tb_errno_key, malloc(sizeof(int)));
	*(int *)pthread_getspecific(tb_errno_key) = 0;
}
 
/* Free the thread-specific buffer */
static void tb_errno_destroy(void *buf) {
	free(buf);                           
}
	
static void tb_errno_key_create() {
	pthread_key_create(&tb_errno_key, tb_errno_destroy);
}








