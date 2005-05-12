//------------------------------------------------------------------
// $Id: Num.c,v 1.6 2005/05/12 21:52:12 plg Exp $
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
#include "Tlv.h"

int OM_NUMSET;

inline num_members_t XNum(Num_t N) {
	return (num_members_t)((__members_t)tb_getMembers(N, TB_NUM))->instance;
}

static Tlv_t        tb_num_toTlv        (Num_t Self);
static Num_t        tb_num_fromTlv      (Tlv_t T);
static Num_t        tb_num_ctor         (Num_t N,int val);
static Num_t        tb_num_new          ();
static void *       tb_num_free         (Num_t N);
static Num_t        tb_num_clone        (Num_t N);
static Num_t        tb_num_clear        (Num_t N);
static int          tb_num_getsize      (Num_t N);
static void         tb_num_dump         (Num_t N, int level) ;
static void         tb_num_marshall     (String_t marshalled, Raw_t R, int level) ;
static Num_t        tb_num_unmarshall   (XmlElt_t xml_entity);
static String_t     tb_num_stringify    (Num_t N);
static cmp_retval_t tb_num_compare      (Num_t N1, Num_t N2);
static Num_t        tb_num_set          (Num_t N, int val);
static Num_t        tb_num_assign       (Num_t N1, Num_t N2);

void __build_num_once(int OID) {
	OM_NUMSET      = tb_registerNew_ClassMethod("NumSet",       OID);

	tb_registerMethod(OID, OM_NEW,          tb_num_new);
	tb_registerMethod(OID, OM_FREE,         tb_num_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_num_getsize);
	tb_registerMethod(OID, OM_CLONE,        tb_num_clone);
	tb_registerMethod(OID, OM_DUMP,         tb_num_dump);
	tb_registerMethod(OID, OM_CLEAR,        tb_num_clear);
	tb_registerMethod(OID, OM_COMPARE,      tb_num_compare);
	tb_registerMethod(OID, OM_SET,          tb_num_assign);

	//	tb_registerMethod(OID, OM_STRINGIFY,    N2sz);
	tb_registerMethod(OID, OM_STRINGIFY,    tb_num_stringify);


	tb_registerMethod(OID, OM_NUMSET,        tb_num_set);

	tb_implementsInterface(OID, "C_Castable", 
												 &__c_castable_build_once, build_c_castable_once);

	tb_registerMethod(OID, OM_TOSTRING,     N2sz);
	tb_registerMethod(OID, OM_TOINT,        N2int);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);
	tb_registerMethod(OID, OM_MARSHALL,     tb_num_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_num_unmarshall);
	tb_registerMethod(OID, OM_TOTLV,        tb_num_toTlv);
	tb_registerMethod(OID, OM_FROMTLV,      tb_num_fromTlv);

}



Num_t dbg_tb_num(char *func, char *file, int line, int val) {
	set_tb_mdbg(func, file, line);
	return tb_num_ctor(tb_num_new(), val);
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
	return tb_num_ctor(tb_num_new(), value);
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
	if(tb_valid(N, TB_NUM, __FUNCTION__)) {
		void *p;

		if((p = tb_getMethod(N, OM_NUMSET))) {
			return ((Num_t(*)(Num_t, int val))p)(N, val);
		} else {
			tb_error("%p (%d:%s) [no numSet method]\n", N, N->isA, tb_nameOf(N->isA));
			set_tb_errno(TB_ERR_NO_SUCH_METHOD);
		}
	}
	return NULL;

}


static Num_t tb_num_set(Num_t N, int val) {
	if(! tb_valid(N, TB_NUM, __FUNCTION__)) return NULL;
	num_members_t m = XNum(N);
  m->value = val;
	m->NaN = 0;
	snprintf(m->strbuff, 20, "%d", val);

	return N;
}

static cmp_retval_t tb_num_compare(Num_t N1, Num_t N2) {
	if(tb_toInt(N1) == tb_toInt(N2)) return TB_CMP_IS_EQUAL;
	return (tb_toInt(N1) > tb_toInt(N2)) ? TB_CMP_IS_GREATER : TB_CMP_IS_LOWER;
}

static Num_t tb_num_assign(Num_t N1, Num_t N2) {
	return tb_num_set(N1, XNum(N2)->value);
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

static Num_t tb_num_new() {
	tb_Object_t This;
	num_members_t m;
	This =  tb_newParent(TB_NUM); 
	
	This->isA = TB_NUM;
	This->members->instance = tb_xcalloc(1, sizeof(struct num_members));
	m = (num_members_t)This->members->instance;
	m->size  = sizeof(int);
	if(fm->dbg) fm_addObject(This);

	return This;
}

static Num_t tb_num_ctor(Num_t Self, int val) {
	num_members_t m = (num_members_t)Self->members->instance;
	m->size  = sizeof(int);
	m->value = val;
	snprintf(m->strbuff, 20, "%d", val);
	return Self;
}


static void *tb_num_free(Num_t N) {
	tb_freeMembers(N);
	N->isA = TB_NUM;
	return tb_getParentMethod(N, OM_FREE);
}

static Num_t tb_num_clone(Num_t N) {
	Num_t M;
	if(! TB_VALID(N, TB_NUM)) return NULL;
	M = tb_num_ctor(tb_num_new(), XNum(N)->value);

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
	XmlElt_t Xe = XELT_getFirstChild(xml_entity);
	if(Xe != NULL) {
		N = tb_Num(tb_toInt(XELT_getText(Xe)));
	} else {
		N = tb_Num(0); // we must have a default, anyway
	}

	return N;
}


Tlv_t tb_num_toTlv(Num_t Self) {
	num_members_t m = XNum(Self);
	return Tlv(TB_NUM, sizeof(int), (char *)&(m->value));
}

Num_t tb_num_fromTlv(Tlv_t T) {
	int val   = *(((int*)T)+2);
	return tb_Num(val);
}

















