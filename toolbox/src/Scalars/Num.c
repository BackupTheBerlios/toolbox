//------------------------------------------------------------------
// $Id: Num.c,v 1.4 2004/06/15 15:08:27 plg Exp $
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

/**
 * @file Num.c 
 */

/**
 * @defgroup Num Num_t
 * @ingroup Scalar
 * Num object related methods and functions
 */

#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "Num.h"
#include "Objects.h"
#include "tb_ClassBuilder.h"
#include "C_Castable_interface.h"
#include "Serialisable_interface.h"

#include "Memory.h"
#include "Error.h"


inline num_members_t XNum(Num_t N) {
	return (num_members_t)((__members_t)tb_getMembers(N, TB_NUM))->instance;
}

static Num_t        tb_num_new          (int val);
static void *       tb_num_free         (Num_t N);
static Num_t        tb_num_clone        (Num_t N);
static Num_t        tb_num_clear        (Num_t N);
static int          tb_num_getsize      (Num_t N);
static void         tb_num_dump         (Num_t N, int level) ;
static void         tb_num_marshall     (String_t marshalled, Raw_t R, int level) ;
static Num_t        tb_num_unmarshall   (XmlElt_t xml_entity);
static String_t     tb_num_stringify    (Num_t N);
static cmp_retval_t tb_num_compare      (Num_t N1, Num_t N2);


void __build_num_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_num_new);
	tb_registerMethod(OID, OM_FREE,         tb_num_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_num_getsize);
	tb_registerMethod(OID, OM_CLONE,        tb_num_clone);
	tb_registerMethod(OID, OM_DUMP,         tb_num_dump);
	tb_registerMethod(OID, OM_CLEAR,        tb_num_clear);
	tb_registerMethod(OID, OM_COMPARE,      tb_num_compare);

	//	tb_registerMethod(OID, OM_STRINGIFY,    N2sz);
	tb_registerMethod(OID, OM_STRINGIFY,    tb_num_stringify);

	tb_implementsInterface(OID, "C_Castable", 
												 &__c_castable_build_once, build_c_castable_once);

	tb_registerMethod(OID, OM_TOSTRING,     N2sz);
	tb_registerMethod(OID, OM_TOINT,        N2int);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

	tb_registerMethod(OID, OM_MARSHALL,     tb_num_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_num_unmarshall);
}



Num_t dbg_tb_num(char *func, char *file, int line, int val) {
	set_tb_mdbg(func, file, line);
	return tb_num_new(val);
}

/** Num_t constructor.
 *
 * store integer value into a new Num_t
 *
 * Methods: 
 * - tb_NumSet
 * @param value : integer to be stored
 * @return newly allocated Num_t
 *  @see Object, Scalar, tb_NumSet
 * @ingroup Num
*/
Num_t tb_Num(int value) {
	return tb_num_new(value);
}


/** Affects new value to Num_t
 *
 * Setup Num_t value
 *
 * @return: Num_t target
 *
 * @warning 
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see: Object, Scalar, Num
 * @ingroup Num
 */
Num_t tb_NumSet(Num_t N, int val) {
	if(! tb_valid(N, TB_NUM, __FUNCTION__)) return NULL;
	num_members_t m = XNum(N);
  m->value = val;
	m->NaN = 0;
	snprintf(m->strbuff, 20, "%d", val);

	return N;
}

static cmp_retval_t tb_num_compare(Num_t N1, Num_t N2) {
	if(XNum(N1)->value == XNum(N2)->value) return TB_CMP_IS_EQUAL;
	return (XNum(N1)->value > XNum(N2)->value) ? TB_CMP_IS_GREATER : TB_CMP_IS_LOWER;
}

static String_t tb_num_stringify(Num_t N) {
	return tb_String("%s", N2sz(N));
}

char *N2sz(Num_t N) {
	if(! TB_VALID(N, TB_NUM)) return NULL;
	
	return XNum(N)->strbuff;
}

int N2int(Num_t N) {
	no_error;
	if(! tb_valid(N, TB_NUM, __FUNCTION__)) return -1;
	num_members_t m = XNum(N);
	
	if(! m->NaN) {
		return m->value;
	}
	set_tb_errno(TB_ERR_UNSET);
	return -1;
}


static int tb_num_getsize(Num_t N) {
	return XNum(N)->size;
}

static Num_t tb_num_new(int val) {
	tb_Object_t This;
	num_members_t m;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	This =  tb_newParent(TB_NUM); 
	
	This->isA = TB_NUM;
	This->members->instance = tb_xcalloc(1, sizeof(struct num_members));
	m = (num_members_t)This->members->instance;
	m->size  = sizeof(int);
	m->value = val;
	snprintf(m->strbuff, 20, "%d", val);
	if(fm->dbg) fm_addObject(This);

	return This;
}

static void *tb_num_free(Num_t N) {
	tb_freeMembers(N);
	tb_freeMembers(N);
	N->isA = TB_NUM;
	return tb_getParentMethod(N, OM_FREE);
}

static Num_t tb_num_clone(Num_t N) {
	Num_t M;
	if(! TB_VALID(N, TB_NUM)) return NULL;
	M = tb_num_new(XNum(N)->value);

	return M;
}

static Num_t tb_num_clear(Num_t N) {
	if(tb_valid(N, TB_NUM, __FUNCTION__)) {
		num_members_t m = XNum(N);
		m->NaN = 1;
		snprintf(m->strbuff, 20, "NaN");
		m->value = 0;
	}
	return N;
}

static void tb_num_dump(Num_t N, int level) {
	if(tb_valid(N, TB_NUM, __FUNCTION__)) {
		int i;
		num_members_t m = XNum(N);
		for(i = 0; i<level; i++) fprintf(stderr, " ");

		fprintf(stderr, "<TB_NUM ADDR=\"%p\" REFCNT=\"%d\" VALUE=\"",
						N, N->refcnt);
		if(m->NaN == 1) {
			fprintf(stderr, "NaN\" />\n");
		} else {
			fprintf(stderr, "%d\" />\n", m->value);
		}
	}
}



static void tb_num_marshall( String_t marshalled, Num_t N, int level) { 
	char indent[level+1];
	if(tb_valid(N, TB_NUM, __FUNCTION__) && 
		 tb_valid(marshalled, TB_STRING, __FUNCTION__))
		{
			num_members_t m = XNum(N);
			memset(indent, ' ', level);
			indent[level] = 0;
			if(m->NaN == 1) {
				tb_StrAdd(marshalled, -1, "%s<int/>\n", indent);
			} else {
				tb_StrAdd(marshalled, -1, "%s<int>%d</int>\n", indent, m->value);
			}
		}
}

static Num_t tb_num_unmarshall(XmlElt_t xml_entity) {
	Num_t N;
	if(! streq(S2sz(XELT_getName(xml_entity)), "int")) {
		tb_warn("tb_num_unmarshall: not an int Elmt\n");
		return NULL;
	}

	N = tb_Num(S2int(XELT_getText(tb_Get(XELT_getChildren(xml_entity), 0))));

	return N;
}




















