//======================================================
// $Id: XmlElt.c,v 1.4 2005/05/12 21:53:10 plg Exp $
//======================================================
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expat.h"

#include "Toolbox.h"
#include "Objects.h"
#include "Composites.h"
#include "Memory.h"
#include "tb_Xml.h"
#include "tb_ClassBuilder.h"

static inline tb_Object_t _tb_dock(tb_Object_t O)  { O->docked++; return O; }
static inline tb_Object_t _tb_undock(tb_Object_t O){ O->docked--; return O; }

void tb_xmlelt_dump(XmlElt_t X, int level);

void __build_xmlelt_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_XmlElt);
	tb_registerMethod(OID, OM_FREE,         tb_xmlelt_free);
	tb_registerMethod(OID, OM_DUMP,         tb_xmlelt_dump);
}

XmlElt_t dbg_tb_xmlnodeelt(char *func, char *file, int line, 
														XmlElt_t parent, char *name, Hash_t Attr) {
	set_tb_mdbg(func, file, line);

	return tb_XmlNodeElt(parent, name, Attr);
} 

/** Xml Node element contructor
 * @ingroup XmlLite
 */
XmlElt_t tb_XmlNodeElt(XmlElt_t parent, char *name, Hash_t Attr) {
	tb_Object_t X;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	if(parent && XELT_getType(parent) != XELT_TYPE_NODE) {
		tb_error("tb_XmlNodeElt: bad parent (adoption impossible)\n");
		return NULL;
	}

	X        = tb_newParent(TB_XMLELT); 
	X->isA   = TB_XMLELT;
	X->members->instance = (xml_obj_t) tb_xcalloc(1, sizeof(struct xml_obj));

	XXELT(X)->ElmType = XELT_TYPE_NODE;
	if(name) { 
		XXELT(X)->Name  = _tb_dock(tb_String("%s", name)); 
	}

	if(parent) {
		XXELT(X)->xml_space = XElt(parent)->xml_space; // use parent default scheme for spaces
	} 

	XXELT(X)->Children   = _tb_dock(tb_Vector());
	if(Attr) {
		if(tb_Exists(Attr, "xml:space")) { // xml:space policy redefined
			if(streq(tb_toStr(Attr, "xml:space"), "preserve")) {
				tb_info("set node [%s] with xml:space='preserve'\n", name);
				XXELT(X)->xml_space = 1; // preserve spaces in inner nodes
			} else if(streq(tb_toStr(Attr, "xml:space"), "default")) {
				XXELT(X)->xml_space = 0; // skip spaces in inner nodes
			}
		}
		XXELT(X)->Attributes = _tb_dock(tb_Clone(Attr));
	}
	XXELT(X)->Parent       = parent;

	if(parent) tb_Push(XELT_getChildren(parent), X);
	if(fm->dbg) fm_addObject(X);

	return X;
} 


XmlElt_t dbg_tb_xmltextelt(char *func, char *file, int line,XmlElt_t parent, char *text) {
	set_tb_mdbg(func, file, line);

	return tb_XmlTextElt(parent, text);
} 


/** Xml Text element contructor
 * @ingroup XmlLite
 */
XmlElt_t tb_XmlTextElt(XmlElt_t parent, char *text) {
	tb_Object_t X;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	if(parent && XELT_getType(parent) != XELT_TYPE_NODE) {
		tb_error("tb_XmlNodeElt: bad parent (adoption impossible)\n");
		return NULL;
	}

	X        = tb_newParent(TB_XMLELT); 
	X->isA   = TB_XMLELT;
	X->members->instance = (xml_obj_t) tb_xcalloc(1, sizeof(struct xml_obj));

	XXELT(X)->ElmType        = XELT_TYPE_TEXT;
	if(text) {
		XXELT(X)->Text         = _tb_dock(tb_String("%s", text));
	} else {
		XXELT(X)->Text         = _tb_dock(tb_String(NULL));
	}
	XXELT(X)->Parent         = parent;

	if(parent) tb_Push(XELT_getChildren(parent), X);
	if(fm->dbg) fm_addObject(X);

	return X;
} 


XmlElt_t tb_XmlCDataElt(XmlElt_t parent, char *text) {
	tb_Object_t X;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	if(parent && XELT_getType(parent) != XELT_TYPE_NODE) {
		tb_error("tb_XmlNodeElt: bad parent (adoption impossible)\n");
		return NULL;
	}

	X        = tb_newParent(TB_XMLELT); 
	X->isA   = TB_XMLELT;
	X->members->instance = (xml_obj_t) tb_xcalloc(1, sizeof(struct xml_obj));

	XXELT(X)->ElmType        = XELT_TYPE_CDATA;
	if(text) {
		XXELT(X)->Text         = _tb_dock(tb_String("%s", text));
	} else {
		XXELT(X)->Text         = _tb_dock(tb_String(NULL));
	}
	XXELT(X)->Parent         = parent;

	if(parent) tb_Push(XELT_getChildren(parent), X);
	if(fm->dbg) fm_addObject(X);

	return X;
} 


XmlElt_t dbg_tb_xmlelt(char *func, char *file, int line,
												int type, XmlElt_t parent, char *name, Hash_t Atts) {
	set_tb_mdbg(func, file, line);
	return tb_XmlElt(type, parent, name, Atts);
} 


/** Xml element contructor
 * @ingroup XmlLite
 */
XmlElt_t tb_XmlElt(int type, XmlElt_t parent, char *name, Hash_t Attr) {
	tb_Object_t X;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	if(type != XELT_TYPE_TEXT && type != XELT_TYPE_NODE && type != XELT_TYPE_CDATA) {
		tb_error("tb_XmlElt: bad element type\n");
		return NULL;
	}

	X =  tb_newParent(TB_XMLELT); 
	X->isA  = TB_XMLELT;

	X->members->instance = (xml_obj_t) tb_xcalloc(1, sizeof(struct xml_obj));

	XXELT(X)->ElmType      = type;
	if(name) {
		XXELT(X)->Name       = _tb_dock(tb_String("%s", name));
		XXELT(X)->Children   = _tb_dock(tb_Vector());
	}
	if(Attr) {
		XXELT(X)->Attributes = _tb_dock(Attr);
	}

	XElt(X)->Parent       = parent;
	if(Attr) {
		if(parent) {
			XXELT(X)->xml_space = XElt(parent)->xml_space; // use parent default scheme for spaces
		} 
		if(tb_Exists(XXELT(X)->Attributes, "xml:space")) { // xml:space policy redefined
			if(streq(tb_toStr(XXELT(X)->Attributes, "xml:space"), "preserve")) {
				XXELT(X)->xml_space = 1; // preserve spaces in inner nodes
			} else if(streq(tb_toStr(XXELT(X)->Attributes, "xml:space"), "default")) {
				XXELT(X)->xml_space = 0; // skip spaces in inner nodes
			}
		}
	}
	if(fm->dbg) fm_addObject(X);

	return X;
} 







void tb_xmlelt_dump(XmlElt_t X, int level) {
	xml_obj_t members;
	if(X == NULL) return;

	members = XElt(X);

	switch(members->ElmType) 
		{
		case XELT_TYPE_TEXT:
			{
				fprintf(stderr, "<XMLELT_T TYPE=\"TEXT\" DOCKED=\"%d\">\n", X->docked);
				
				tb_Dump(members->Text);
				fprintf(stderr, "</XMLELT_T> <!-- text -->\n");
				break;
			}
		case XELT_TYPE_CDATA:
			{
				fprintf(stderr, "<XMLELT_T TYPE=\"CDATA\" DOCKED=\"%d\">\n", X->docked);
				tb_Dump(members->Text);
				fprintf(stderr, "<XMLELT_T>\n");
				break;
			}
		case XELT_TYPE_NODE:
			{
				if(members->xml_space) {
					fprintf(stderr, 
									"<XMLELT_T TYPE=\"NODE\" xml:space=\"preserve\" DOCKED=\"%d\">\n", X->docked);
				} else {
					fprintf(stderr, "<XMLELT_T TYPE=\"NODE\" DOCKED=\"%d\">\n", X->docked);
				}
				tb_Dump(members->Attributes);
				tb_Dump(members->Children);
				fprintf(stderr, "</XMLELT_T> <!-- node -->\n");

			}
		} 
}


void *tb_xmlelt_free(XmlElt_t X) {
	XmlElt_t T;

	if(tb_valid(X, TB_XMLELT, __FUNCTION__)) {
		xml_obj_t members = XElt(X);

		if(members->Name)       tb_Free(_tb_undock(members->Name));
		if(members->Attributes) tb_Free(_tb_undock(members->Attributes));
		if(members->Text)       tb_Free(_tb_undock(members->Text));
		if(members->Children) {
			while((T = tb_Shift(members->Children))) { // FIXME: why not remove all at once ?
				tb_Free(T);
			}
			tb_Free(_tb_undock(members->Children)); 
		}
		tb_freeMembers(X);
		X->isA = TB_XMLELT;
		return tb_getParentMethod(X, OM_FREE);
	}
	return NULL;
}


/** Append XML representation of XmlElt internal construct into String_t
 * @ingroup XmlLite
 */
void XmlElt_to_xml(String_t Rez, XmlElt_t X) {
	int       i, max;
	xml_obj_t members;

	if(X == NULL|| Rez == NULL) return;
	members = XElt(X);
	
	switch(members->ElmType) 
		{
		case XELT_TYPE_TEXT:
			{
				tb_StrAdd(Rez, -1, "%s", S2sz(members->Text));
				break;
			}
		case XELT_TYPE_CDATA:
			{
				tb_StrAdd(Rez, -1, "<![CDATA[%s]]>", S2sz(members->Text));
				break;
			}
		case XELT_TYPE_NODE:
			{
				tb_StrAdd(Rez, -1, "<%s ", S2sz(members->Name));
				max = tb_getSize(members->Children);
	
				if(tb_getSize(members->Attributes)) {
					Iterator_t It = tb_Iterator(members->Attributes);
					tb_First(It);
					do {
						tb_StrAdd(Rez, -1, "%s=\"%s\" ", 
											tb_Key(It).key, 
											S2sz(tb_Value(It))); 
					} while(tb_Next(It));
					tb_Free(It);
				}

				if(max > 0) {
					tb_StrAdd(Rez, -1, ">"); // close opening tag
					for(i = 0; i<max;  i++) {
						XmlElt_to_xml(Rez, tb_Get(members->Children, i));// dump childs
					}
					tb_StrAdd(Rez, -1, "</%s>\n", S2sz(members->Name)); // add closing tag

				} else {
					tb_StrAdd(Rez, -1, "/>\n"); // opening tag is also closing tag
				} 

				break;
			}
		} 
}





/** Returns Element's Children
 * @ingroup XmlLite
 */
Vector_t XELT_getChildren(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && XElt(X)->ElmType == XELT_TYPE_NODE) {
		return XElt(X)->Children;
	}
	return NULL;
}

/** Returns Element's Parent
 * @ingroup XmlLite
 */
XmlElt_t XELT_getParent(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__)) {
		return XElt(X)->Parent;
	}
	return NULL;
}	


/** Returns Element's name
 * @ingroup XmlLite
 */
String_t XELT_getName(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && XElt(X)->ElmType == XELT_TYPE_NODE) {
		return XElt(X)->Name;
	}
	return NULL;
}	



/** Sets Element's name
 * @ingroup XmlLite
 */
int XELT_setName(XmlElt_t X, char *name) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && XElt(X)->ElmType == XELT_TYPE_NODE) {
		tb_StrAdd(tb_Clear(XElt(X)->Name), 0 , "%s", name);
		return TB_OK;
	}
	return TB_KO;
}	



/** Returns Element's Attributes
 * @ingroup XmlLite
 */
Hash_t XELT_getAttributes(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && XElt(X)->ElmType == XELT_TYPE_NODE) {
		return XElt(X)->Attributes;
	}
	return NULL;
}	



/** Returns Element's named attribute
 * @ingroup XmlLite
 */
tb_Object_t XELT_getAttribute(XmlElt_t X, char *attr) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && XElt(X)->ElmType == XELT_TYPE_NODE) {
		return tb_Get(XElt(X)->Attributes, attr);
	}
	return NULL;
}	


/** Add an attribute to Element
 * @ingroup XmlLite
 */
int XELT_addAttributes(XmlElt_t X, Hash_t newAttr) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && 
		 XElt(X)->ElmType == XELT_TYPE_NODE &&
		 tb_valid(newAttr, TB_HASH, __FUNCTION__)) 
		{
			xml_obj_t members = XElt(X);
			if(tb_getSize(newAttr)) {
				Iterator_t It = tb_Iterator(newAttr);
				tb_First(It);
				do {
					tb_Replace(members->Attributes, tb_Clone(tb_Value(It)), tb_Key(It));
				} while(tb_Next(It));
				tb_Free(It);
			}
			return TB_OK;
		}
	return TB_KO;
}


/** Get Element type
 * @ingroup XmlLite
 */
int XELT_getType(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__)) {
		return XElt(X)->ElmType;
	}
	return -1;
}	


/** Get Element text
 * @ingroup XmlLite
 */
String_t XELT_getText(XmlElt_t X) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && 
		 (XElt(X)->ElmType == XELT_TYPE_TEXT ||
			 XElt(X)->ElmType == XELT_TYPE_CDATA)) {
		return XElt(X)->Text;
	}
	return NULL;
}	

/** Get Element text, including embedded text tags (deep concat)
 * @ingroup XmlLite
 */
String_t XELT_getAllText(XmlElt_t X) {
	String_t S;
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && 
		 (XElt(X)->ElmType == XELT_TYPE_TEXT ||
			 XElt(X)->ElmType == XELT_TYPE_CDATA)) {
		Vector_t C = XELT_getChildren(X);
		int i, max = tb_getSize(C);
		S = tb_String(NULL);
		for(i=0; i<max; i++) {
			XmlElt_t O = tb_Get(C, i);
			if(XELT_getType(O) == XELT_TYPE_TEXT) {
				tb_StrAdd(S, -1, "%s", S2sz(XELT_getText(O)));
			}
		}
		return S;
	}
	return NULL;
}	


/** Set Element text
 * @ingroup XmlLite
 */
int XELT_setText(XmlElt_t X, char *text) {
	if(tb_valid(X, TB_XMLELT, __FUNCTION__) && 
		 (XElt(X)->ElmType == XELT_TYPE_TEXT ||
			 XElt(X)->ElmType == XELT_TYPE_CDATA)) {
		tb_StrAdd(tb_Clear(XElt(X)->Text), 0, "%s", text);
		return TB_OK;
	}
	return TB_KO;
}



XmlElt_t XELT_getFirstChild(XmlElt_t xml) {
	Vector_t V = XELT_getChildren(xml);
	if(V && tb_getSize(V) >0) {
		return tb_Get(V,0);
	}
	return NULL;
}

String_t XELT_getTextChild(XmlElt_t xml) {
	return XELT_getText(XELT_getFirstChild(xml));
}

