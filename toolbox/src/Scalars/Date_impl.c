/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//------------------------------------------------------------------
// $Id: Date_impl.c,v 1.1 2004/05/12 22:04:53 plg Exp $
//------------------------------------------------------------------
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

// created on Sun May  9 15:48:00 2004 by Paul Gatille <paul.gatille@free.fr>


// Class Date (extends TB_SCALAR, [implements interface ...])
// description :


/* uncomment only for internal Toolbox Class 
#ifndef BUILD
#define BUILD
#endif
*/

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Date.h"
#include "Date_impl.h"
#include "tb_ClassBuilder.h"

int TB_DATE;

static void *Date_free( Date_t Obj);
//static  Date_t Date_clone(Date_t Obj);
//static  void Date_dump(Date_t Obj);
//static  Date_t Date_clear(Date_t Obj);
//...

void setup_Date_once(int OID);

inline Date_members_t XDate(Date_t T) {
	return (Date_members_t)((__members_t)tb_getMembers(T, DATE_T))->instance;
}

pthread_once_t __init_Date_once = PTHREAD_ONCE_INIT;
void init_Date_once() {
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	// if you doesn't extends directly from a toolbox class, you must call your anscestor init
	DATE_T = tb_registerNewClass("TB_DATE", TB_SCALAR, setup_Date_once);
}

void setup_Date_once(int OID) {
	/* OM_NEW and OM_FREE are mandatory methods */
	tb_registerMethod(OID, OM_NEW,                    Date_new);
	tb_registerMethod(OID, OM_FREE,                   Date_free);
	/*  others are optionnal: this ones are common and shown as example */
//	tb_registerMethod(OID, OM_CLONE,                  Date_clone);
//	tb_registerMethod(OID, OM_DUMP,                   Date_dump);
//	tb_registerMethod(OID, OM_CLEAR,                  Date_clear);
}


Date_t dbg_Date(char *func, char *file, int line, char *iso8601) {
	set_tb_mdbg(func, file, line);
	return Date_new(iso8601);
}


 /** Date_t constructor
 *
 * oneliner header function description
 *
 * longer description/explanation
 *
 * @param arg1 desc of arg1
 * @param argn desc of argn
 * @return descrition of return values
 *
 * @see related entries
 * \ingroup DATE
 */

Date_t Date(char *iso8601) {
	return Date_new(iso8601);
}



Date_t Date_new(char *iso8601) {
	tb_Object_t This;
	pthread_once(&__init_Date_once, init_Date_once);
	Date_members_t m;
	This =  tb_newParent(TB_DATE); 
	
	This->isA  = TB_DATE;



	m = (Date_members_t)xcalloc(1, sizeof(struct Date_members));
	This->members->instance = m;

	if((m->absolute = iso8601_to_time(iso8601)) >0) {
		localtime_r(&(m->absolute), m->broken_down);
	}

	if(fm->dbg) fm_addObject(This);

	return This;
}

void *Date_free(Date_t Obj) {
	if(tb_valid(Obj, DATE_T, __FUNCTION__)) {

		fm_fastfree_on();

		tb_freeMembers(Obj);
		fm_fastfree_off();
		Obj->isA = DATE_T; // requiered for introspection (as we are unwinding dtors stack)
    return tb_getParentMethod(Obj, OM_FREE);
	}

  return NULL;
}

/*
static Date_t Date_clone(Date_t This) {
}
*/

/*
static void Date_dump(Date_t This) {
}
*/



