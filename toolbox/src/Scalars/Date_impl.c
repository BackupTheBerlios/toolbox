/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//------------------------------------------------------------------
// $Id: Date_impl.c,v 1.2 2004/05/13 22:07:05 plg Exp $
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

#include "Date_impl.h"
#include "Memory.h"
#include "Error.h"
#include "tb_ClassBuilder.h"
#include "C_Castable_interface.h"
#include "Serialisable_interface.h"


static void   * Date_free    (Date_t This);
static Date_t   Date_clone   (Date_t This);
static void     Date_dump    (Date_t This, int level);
static Date_t   Date_clear   (Date_t This);
static int      Date_toInt   (Date_t This);
static char   * Date_toStr   (Date_t This);


void setup_Date_once(int OID);

inline Date_members_t XDate(Date_t T) {
	return (Date_members_t)((__members_t)tb_getMembers(T, TB_DATE))->instance;
}


void __build_date_once(int OID) {
	/* OM_NEW and OM_FREE are mandatory methods */
	tb_registerMethod(OID, OM_NEW,                    Date_new);
	tb_registerMethod(OID, OM_FREE,                   Date_free);
	/*  others are optionnal: this ones are common and shown as example */
	tb_registerMethod(OID, OM_CLONE,                  Date_clone);
	tb_registerMethod(OID, OM_DUMP,                   Date_dump);
	tb_registerMethod(OID, OM_CLEAR,                  Date_clear);

	tb_implementsInterface(OID, "C_Castable", 
												 &__c_castable_build_once, build_c_castable_once);
	
	tb_registerMethod(OID, OM_TOSTRING,     Date_toStr);
	tb_registerMethod(OID, OM_TOINT,        Date_toInt);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

/* 	tb_registerMethod(OID, OM_MARSHALL,     Date_marshall); */
/* 	tb_registerMethod(OID, OM_UNMARSHALL,   Date_unmarshall); */

}


Date_t dbg_tb_Date(char *func, char *file, int line, char *iso8601) {
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

Date_t tb_Date(char *iso8601) {
	return Date_new(iso8601);
}



Date_t Date_new(char *iso8601) {
	tb_Object_t This;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	Date_members_t m;
	This =  tb_newParent(TB_DATE); 
	
	This->isA  = TB_DATE;

	m = (Date_members_t)tb_xcalloc(1, sizeof(struct Date_members));
	This->members->instance = m;

	if((m->absolute = iso8601_to_time(iso8601)) >0) {
		localtime_r(&(m->absolute), &(m->broken_down));

		snprintf(m->string, 20, "%d%02d%02dT%02d:%02d:%02d",
						 m->broken_down.tm_year + 1900,
						 m->broken_down.tm_mon,
						 m->broken_down.tm_mday,
						 m->broken_down.tm_hour,
						 m->broken_down.tm_min,
						 m->broken_down.tm_sec);
		
	}

	if(fm->dbg) fm_addObject(This);

	return This;
}

void *Date_free(Date_t Obj) {
	fm_fastfree_on();

	tb_freeMembers(Obj);
	fm_fastfree_off();
	Obj->isA = TB_DATE; // requiered for introspection (as we are unwinding dtors stack)
	return tb_getParentMethod(Obj, OM_FREE);
}

static Date_t Date_clear(Date_t This) {
	Date_members_t m = XDate(This);
	m->broken_down.tm_year  = 0;
	m->broken_down.tm_mon   = 0;
	m->broken_down.tm_mday  = 0;
	m->broken_down.tm_hour  = 0;
	m->broken_down.tm_min   = 0;
	m->broken_down.tm_sec   = 0;
	m->absolute = 0;
	m->string[0]=0;
	return This;
}

static Date_t Date_clone(Date_t This) {
	return tb_Date(Date_toStr(This));
}

static void Date_dump(Date_t This, int level) {
	int i;
	Date_members_t m = XDate(This);
  for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, 
					"<TB_DATE ADDR=\"%p\" DATE=\"%s\" REFCNT=\"%d\" DOCKED=\"%d\">\n",
					m, m->string,
					This->refcnt, 
					This->docked);
}



static char * Date_toStr(Date_t This) {
	return XDate(This)->string;
}

static int Date_toInt(Date_t This) {
	if(tb_valid(This, TB_DATE, __FUNCTION__)) {
		Date_members_t m = XDate(This);
		return m->absolute;
	}
	return 0;
}
