//------------------------------------------------------------------
// $Id: Raw.c,v 1.1 2004/05/12 22:04:52 plg Exp $
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
 * @file Raw.c 
 */

/**
 * @defgroup Raw Raw_t
 * @ingroup Scalar
 * Raw object related methods and functions
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "Toolbox.h"
#include "tb_global.h"
#include "Raw.h"
#include "tb_ClassBuilder.h"
#include "Serialisable_interface.h"

#include "Memory.h"

inline raw_members_t XRaw(Raw_t R) {
	return (raw_members_t)((__members_t)tb_getMembers(R, TB_RAW))->instance;
}

static Raw_t        tb_raw_new   (int len, char *data);
static void       * tb_raw_free  (Raw_t R);
static void         tb_raw_dump  (Raw_t R, int level);
static Raw_t        tb_raw_clone (Raw_t R);
static int          tb_raw_getsize(Raw_t R);
static Raw_t        tb_raw_clear (Raw_t R);
static void         tb_raw_marshall( String_t marshalled, Raw_t R, int level);
static Raw_t        tb_raw_unmarshall(XmlElt_t xml_entity);

void __build_raw_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_raw_new);
	tb_registerMethod(OID, OM_FREE,         tb_raw_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_raw_getsize);
	tb_registerMethod(OID, OM_CLONE,        tb_raw_clone);
	tb_registerMethod(OID, OM_DUMP,         tb_raw_dump);
	tb_registerMethod(OID, OM_CLEAR,        tb_raw_clear);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

	tb_registerMethod(OID, OM_MARSHALL,     tb_raw_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_raw_unmarshall);
}




Raw_t dbg_tb_raw(char *func, char *file, int line, int len, char *data) {
	set_tb_mdbg(func, file, line);
	return tb_raw_new(len, data);
}


/** Raw_t constructor.
 *
 * Allocate and initialise a container for binary data
 * 
 * Raw_t is a kind of limited String_t, best fitted to store binary data (including 0x00 bytes).
 *
 * @param len : max len of source data to be copied
 * @param data : source data
 * @return newly allocated Raw_t
 * @see Object, Scalar, tb_hexdump, tb_str2hex
 * @ingroup Raw
 */
Raw_t tb_Raw(int len, char *data) {
	Raw_t R = NULL;
	if( data && len >0 ) {
		R = tb_raw_new(len, data);
	}
	return R;
}

static Raw_t tb_raw_new(int len, char *data) {
	tb_Object_t O;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	O = tb_newParent(TB_RAW); 
	
	O->isA = TB_RAW;
	O->members->instance = (raw_members_t)tb_xcalloc(1, sizeof(struct raw_members));

	if(len >0) {
		raw_members_t m = XRaw(O);
		m->size = len;
		m->data = tb_xcalloc(1, len +1);
		memcpy(m->data, data, len);
	}

	if(fm->dbg) fm_addObject(O);

	return O;
}

static int tb_raw_getsize(Raw_t R) {
	return XRaw(R)->size;
}

static void *tb_raw_free(Raw_t R) {
	if(tb_valid(R, TB_RAW, __FUNCTION__)) {
		raw_members_t m = XRaw(R);
		if(m->data) tb_xfree(m->data);
	}
	tb_freeMembers(R);
	return tb_getParentMethod(R, OM_FREE);
}

static Raw_t tb_raw_clone(Raw_t R) {
	if(tb_valid(R, TB_RAW, __FUNCTION__)) {
		raw_members_t m = XRaw(R);
		return tb_Raw(m->size, m->data);
	}
	return NULL;
}

static Raw_t tb_raw_clear(Raw_t R) {
	if(tb_valid(R, TB_RAW, __FUNCTION__)) {
		raw_members_t m = XRaw(R);
		if(m->data) tb_xfree(m->data);
		m->data = NULL;
		m->size = 0;
	}
	return R;
}

static void tb_raw_dump(Raw_t R, int level ) {
	int               i;
	char            * s;
	raw_members_t     m;

	if(tb_valid(R, TB_RAW, __FUNCTION__))	return;
	
  for(i = 0; i<level; i++) fprintf(stderr, " ");
	m = XRaw(R);

	fprintf(stderr, "<TB_RAW SIZE=\"%d\" ADDR=\"%p\" DATA=\"%p\" REFCNT=\"%d\" ",
					m->size, R, m->data, R->refcnt);
	if(m->size >0) {
		s = tb_str2hex((char *)m->data, m->size);
		fprintf(stderr, ">\n%s</TB_RAW>\n", s);
		tb_xfree(s);
	} else {
		fprintf(stderr, "/>\n");
	}
}

static void tb_raw_marshall( String_t marshalled, Raw_t R, int level) {
	char indent[level+1];
	if(tb_valid(R, TB_RAW, __FUNCTION__) &&
		 tb_valid(marshalled, TB_STRING, __FUNCTION__)) 
		{
			raw_members_t m = XRaw(R);
			memset(indent, ' ', level);
			indent[level] = 0;

			if(m->size >0) {
				char *r;
				tb_EncodeBase64(m->data, m->size, &r);
				tb_StrAdd(marshalled, -1, "%s<base64>%s</base64>\n", indent, r);
				tb_xfree(r);
			} else {
				tb_StrAdd(marshalled, -1, "%s</base64>\n", indent);
			}
		}
}

static Raw_t tb_raw_unmarshall(XmlElt_t xml_entity) {
	Raw_t   R;
	void  * r;
	int     len;

	if(! streq(S2sz(XELT_getName(xml_entity)), "base64")) {
		tb_error("tb_raw_unmarshall: not a base64 Elmt\n");
		return NULL;
	}
	len = tb_DecodeBase64(S2sz(XELT_getText(tb_Get( XELT_getChildren(xml_entity), 0))), &r);

	R = tb_Raw(len, r);
	tb_xfree(r);

	return R;
}

/** Create a hexadecimal representation of binary data
 *
 * Allocate a C string, and fill it with the hexa representation of len bytes of 'bin' data. 
 * Freeing this string is under caller responsability.
 *
 * @param bin : pointer to data
 * @param len : number of bytes to dump
 * @return: allocated C string. 
 * @see Object, Scalar, tb_hexdump tb_str2hex
 * @ingroup Raw
 */
char *tb_str2hex(char *bin, int len) {
	char *rez = tb_xcalloc(1, len*2 +1);
	char *s   = rez;
	int i;
	for(i=0; i<len; i++) {
		char buf[3];
		snprintf(buf, 3, "%02X", (unsigned char)bin[i]);
		memcpy(s + (i*2), buf, 2);
	}
	return rez;
}


/** Dumps an hexadecimal representation of binary data on stderr
 *
 * Output hexa dump on stderr, with offsets, and separated ascii and hexa display.
 *
 * @param bin : pointer to data
 * @param len : number of bytes to dump
 * @return: allocated C string. 
 * @see Object, Scalar, tb_hexdump tb_str2hex
 * @ingroup Raw
 */
void tb_hexdump(char *bin, int len) {

	char str[21];
	int offs = 0;
	
	int max, j,i, lines = len / 16;
	int oldlevel = tb_errorlevel;
	tb_errorlevel = TB_CRIT; // FIXME: global flag; not multithread safe

	if(lines * 16 <len) lines++;

	fprintf(stderr, "-- tb_dumping %d bytes --\n", len);

	fprintf(stderr,
					"----------+------------------+-------------------------------------------------\n");
	for(i = 0; i < lines; i++) {
		char *s;
		String_t S;
		max = TB_MIN(len - offs, 16);
		for(j = 0; j < max; j++) {
			str[j] = (isprint(bin[offs+j])) ? bin[offs+j] : '.'; 
		}
		str[j] = 0;
		S = tb_String("%s", s = tb_str2hex(bin+offs, max));
		tb_Sed("([\\dA-F]{2})", "$1 ", S, PCRE_MULTI);
		tb_trim(S);
		fprintf(stderr, "%p | %-16s | %-45s \n",
						bin+offs,
						str,
						S2sz(S)	);
		tb_xfree(s);
		tb_Free(S);
		offs += max;
	}
	fprintf(stderr,
					"----------+------------------+-------------------------------------------------\n");
	tb_errorlevel = oldlevel;
}	


