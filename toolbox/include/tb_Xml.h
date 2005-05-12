//------------------------------------------------------------------
// $Id: tb_Xml.h,v 1.2 2005/05/12 21:54:36 plg Exp $
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

#ifndef __TB_XML_H
#define __TB_XML_H

#include <Toolbox.h>
#include "expat.h"

struct xml_obj {
	int             ElmType;    
	int             xml_space;    
	String_t        Name;
	Hash_t          Attributes;
	Vector_t        Children;
	String_t        Text;         // valid only if ElmType = XELT_TYPE_TEXT || XELT_TYPE_CDATA
	XmlElt_t        Parent;
};
typedef struct xml_obj *xml_obj_t;

struct xml_tree {
	XML_Parser parser;
	int        xml_space;
	String_t   namespace;
	XmlElt_t   root;
	XmlElt_t   cur;
	int        in_cdata;
};
typedef struct xml_tree *xml_tree_t;

#define INVALID 0x80000000

#define uget(c)  c = *strptr++; \
        if (chars) (*chars)++; \
        if ((c) == 0) return (unsigned char)EOF

// _only_ for internal use ! (doesn't respect inheritance)
#define XXDOC(A)   ((xml_tree_t)A->members->instance)
#define XXELT(A)   ((xml_obj_t)A->members->instance)


inline xml_tree_t XDoc(XmlDoc_t X);
inline xml_obj_t  XElt(XmlElt_t X);

void     * tb_xmldoc_free    (XmlDoc_t X);
void     * tb_xmlelt_free    (XmlElt_t X);
XmlElt_t   new_XmlElt        (xml_obj_t elt);

void       XmlElt_to_xml     (String_t Rez, XmlElt_t X);

#endif


