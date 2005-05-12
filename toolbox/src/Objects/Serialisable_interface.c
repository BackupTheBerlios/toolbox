/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Serialisable_interface.c,v 1.3 2005/05/12 21:51:51 plg Exp $
//======================================================

// created on Thu Aug  1 15:23:25 2002
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


// interface Serialisable : object can be transformed to and from a flat representation
// mainly useful for persistent storage and remote IPCs. 
// Current implementation use xmlrpc conventions (see xmlrpc.org).


#include <pthread.h>
#include <string.h>

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Objects.h"
#include "tb_ClassBuilder.h"
#include "classRegistry.h" // fixme:  __getMethod should be public (in Toolbox building process)
#include "Serialisable_interface.h"
#include "tb_Xml.h"

int IFACE_SERIALISABLE;
int OM_MARSHALL;
int OM_UNMARSHALL;

int OM_TOTLV;
int OM_FROMTLV;

pthread_once_t __serialisable_build_once = PTHREAD_ONCE_INIT;   

void build_serialisable_once() {
	IFACE_SERIALISABLE = tb_registerNewInterface("Serialisable");

	OM_MARSHALL    = tb_registerNew_InterfaceMethod("Marshall",   IFACE_SERIALISABLE);
	OM_UNMARSHALL  = tb_registerNew_InterfaceMethod("UnMarshall", IFACE_SERIALISABLE);

	OM_TOTLV       = tb_registerNew_InterfaceMethod("toTlv",      IFACE_SERIALISABLE);
	OM_FROMTLV     = tb_registerNew_InterfaceMethod("fromTlv",    IFACE_SERIALISABLE);

}

/**
 * Serialize a tb_Object into an xml string
 * \ingroup Object
 *
 *
 * @param O the target object or container
 * @return new String_t containing xml serialisation or NULL if failed. The serialization is always deep (contents of containers are serialized also, even nested containers). 
 *
 *
 * @see Object
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_Share, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt, tb_unMarshall
 * \todo should take a serialize_mode arg to allow future extension (SOAP serialization, binary serialisation, ...)
*/
String_t tb_Marshall(tb_Object_t O) {
	String_t marshalled = NULL;
	if( __VALID_IFACE(tb_isA(O), IFACE_SERIALISABLE)) {
		void *p = tb_getMethod(O, OM_MARSHALL);
		marshalled = tb_String(NULL);
		if(p) {
			((void(*)(String_t, tb_Object_t, int))p)(marshalled, O, 0);
			return marshalled;
		}
	} 
	tb_error("tb_Marshall: Marshalling not implemented for class <%s>\n", 
					 tb_nameOf(tb_isA(O)));
	return NULL;
}


/**
 * build new object by unSerializing an xml representation of tb_Object 
 * \ingroup Object
 *
 *
 * @param marshalled the target xml String_t
 * @return new Object_t or NULL if failed. 
 *
 * @see Object
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_Share, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt, tb_Marshall
*/
tb_Object_t tb_unMarshall(String_t marshalled) {
	XmlDoc_t D;
	XmlElt_t xml_element;
	tb_Object_t O = NULL;

	D = tb_XmlDoc(S2sz(marshalled));

	xml_element = XDOC_getRoot(D);
	O = tb_XmlunMarshall(xml_element);
	tb_Free(D);
	return O;
}

tb_Object_t tb_XmlunMarshall(XmlElt_t xml_element) {
	tb_Object_t (*p)(XmlElt_t) = NULL;
 
	if(tb_StrEQ(XELT_getName(xml_element),        "string")) {
		p = __getMethod(TB_STRING, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "int")) {
		p = __getMethod(TB_NUM, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "boolean")) {
		p = __getMethod(TB_BOOL, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "base64")) {
		p = __getMethod(TB_RAW, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "dateTime.iso8601")) {
		p = __getMethod(TB_DATE, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "date")) {
		p = __getMethod(TB_DATE, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "array")) {
		p = __getMethod(TB_VECTOR, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "struct")) {
		p = __getMethod(TB_HASH, OM_UNMARSHALL);
	} else if(tb_StrEQ(XELT_getName(xml_element), "any")) {
		return tb_Pointer(NULL, NULL);
	} else {
		// Try to find a regular object's method
		int Cid = tb_getClassIdByName(tb_toStr(XELT_getName(xml_element)));
		if(Cid != -1) {
			p = __getMethod(Cid, OM_UNMARSHALL);
	} else {
		tb_warn("unmarshalling pb (unknown type <%s>)\n", S2sz(XELT_getName(xml_element)));
	}
	}
	if(p) return p(xml_element);

	return NULL;
}


Tlv_t tb_toTlv(tb_Object_t O) {
	if( __VALID_IFACE(tb_isA(O), IFACE_SERIALISABLE)) {
		void *p = tb_getMethod(O, OM_TOTLV);
		if(p) {
			return ((Tlv_t(*)(tb_Object_t))p)(O);
		}
	} 
	tb_error("tb_toTlv: tb_toTlv not implemented for class <%s>\n", 
					 tb_nameOf(tb_isA(O)));
	return NULL;
}

tb_Object_t tb_fromTlv(Tlv_t T) {
	if(T==NULL) return NULL;
	int type = Tlv_getType(T); //*(int*)T;
	//	tb_warn("tb_fromTlv: %p <%s>\n", T,  tb_nameOf(type));
	if(__VALID_IFACE(type, IFACE_SERIALISABLE)) {
		void *p = __getMethod(type, OM_FROMTLV);
		if(p) {
			return ((tb_Object_t(*)(Tlv_t))p)(T);
		}
	} 
	tb_error("tb_fromTlv not implemented for class <%s>\n", 
					 tb_nameOf(type));
	//	tb_hexdump(T, Tlv_getFullLen(T));
	return NULL;
}
