//======================================================
// $Id: XmlDoc.c,v 1.4 2005/05/12 21:53:10 plg Exp $
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

/**
 * @file XmlDoc.c XmlLite doc enveloppe related functions
 */

/** @defgroup XmlLite XmlLite
 * Limited XML function kit.
 * allow to parse xml, build a DOM like tree, access and manipulate internal construct, and build xml represenation of internal construct. 
 * Not strictly DOM compliant, but good liteweight compatibility
 * @ingroup Composite
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "expat.h"

#include "Toolbox.h"
#include "Objects.h"
#include "Composites.h"
#include "Memory.h"
#include "tb_Xml.h"
#include "tb_ClassBuilder.h"



inline xml_tree_t XDoc(XmlDoc_t X) {
	return (xml_tree_t)((__members_t)tb_getMembers(X, TB_XMLDOC))->instance;
}

inline xml_obj_t XElt(XmlElt_t X) {
	return (xml_obj_t)((__members_t)tb_getMembers(X, TB_XMLELT))->instance;
}


static void startElement  (void *userData, const char *name, const char **atts);
static void endElement    (void *userData, const char *name);
static void charElement   (void *userData, const char *string, int len);
static void endCData      (void *userData);
static void startCData    (void *userData);

void __build_xmldoc_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_XmlDoc);
	tb_registerMethod(OID, OM_FREE,         tb_xmldoc_free);
}

XmlDoc_t dbg_tb_xmldoc(char *func, char *file, int line,char *xmltext) {
	set_tb_mdbg(func, file, line);
	return tb_XmlDoc(xmltext);
}


/**
 * XmlDoc_t constructor.
 *
 * XmlDoc is the top-level enveloppe for an internal representation of an xml document
 * @param xmltext : xml document to parse, or NULL
 * @return new XmlDoc object
 * @see Composite
 * @ingroup XmlLite
 */
XmlDoc_t tb_XmlDoc(char *xmltext) {
	XmlDoc_t X;
	xml_tree_t tree;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	X = tb_newParent(TB_XMLDOC); 
	
	X->isA  = TB_XMLDOC;

	X->members->instance = (xml_tree_t)tb_xcalloc(1, sizeof(struct xml_tree));
	tree = X->members->instance;


	if(xmltext != NULL) {
		// first find out encoding if specified
		//<?xml version="1.0" encoding="UTF-8"?>
		char *s = NULL;
		if((s = strstr(xmltext, "xml version=\"1.0\" encoding=\""))) {
			tb_notice("found encoding\n");
			if(strneq(s+28, "UTF-8",5)) {
				tb_notice("XmlDoc: encoding set to UTF-8\n");
				tree->parser   = XML_ParserCreate("UTF-8");
			} else if(strneq(s+28, "UTF-16",6)) {
				tb_notice("XmlDoc: encoding set to UTF-16\n");
				tree->parser   = XML_ParserCreate("UTF-16");
			} else {
				tree->parser   = XML_ParserCreate("ISO-8859-1");
				tb_notice("XmlDoc: encoding default to ISO-8859-1\n");
			}
		} else {
			tree->parser   = XML_ParserCreate("ISO-8859-1");
			tb_notice("XmlDoc: encoding default to ISO-8859-1 (2)\n");
		}

		tree->cur      = tree->root;				

		XML_SetUserData(tree->parser, X);
		XML_SetElementHandler(tree->parser, startElement, endElement);
		XML_SetCharacterDataHandler(tree->parser, charElement);

			 XML_SetCdataSectionHandler(tree->parser, startCData, endCData);
		/*
			 XML_SetProcessingInstructionHandler(tree->parser, pi_handler);
			 XML_SetCommentHandler(tree->parser, comment_handler);
		*/

		if (!XML_Parse(tree->parser, xmltext, strlen(xmltext), 0)) {
			tb_error("XmlParse: %s at line %d\n",
							 XML_ErrorString(XML_GetErrorCode(tree->parser)),
							 XML_GetCurrentLineNumber(tree->parser));
			XML_ParserFree(tree->parser);
			tree->parser = NULL;
			tb_Free(X);
			return NULL;
		}

		XML_ParserFree(tree->parser);
		tree->parser = NULL;

		if(tree->cur != NULL) {
			tb_error("XmlParse: not well formed (unclosed element %s)\n",
							 S2sz(XElt(tree->cur)->Name));
			tb_Free(X);
			return NULL;
		}

	}
	if(fm->dbg) fm_addObject(X);

	return X;
}


void *tb_xmldoc_free(XmlDoc_t X) {
	if(tb_valid(X, TB_XMLDOC, __FUNCTION__)) {
		xml_tree_t members = XXDOC(X);
		if(members->namespace)  tb_Free(members->namespace);
		if(members->root)  tb_Free(members->root);

		tb_freeMembers(X);
		X->isA = TB_XMLDOC;
		return tb_getParentMethod(X, OM_FREE);
	}
	return NULL;
}


/** Get xml root node of document
 * @ingroup XmlLite
 */
XmlElt_t XDOC_getRoot(XmlDoc_t X) {
	if(tb_valid(X, TB_XMLDOC, __FUNCTION__)) {
		return XDoc(X)->root;
	}
	return NULL;
}

/** set document Root node
 * @ingroup XmlLite
 */
retcode_t XDOC_setRoot(XmlDoc_t X, XmlElt_t root) {
	if(tb_valid(X, TB_XMLDOC, __FUNCTION__ ) &&
		 tb_valid(root, TB_XMLELT, __FUNCTION__) &&
		 XELT_getType(root) == XELT_TYPE_NODE)
		{
			xml_tree_t members = XDoc(X);
			if(members->root) {
				tb_Free(members->root);
			}
			members->root = root;
			return TB_OK;
		}
	return TB_KO;
}


/** Convert internal construct to xml String_t
 * @ingroup XmlLite
 */
String_t XDOC_to_xml(XmlDoc_t Doc) {
	if(tb_valid(Doc, TB_XMLDOC, __FUNCTION__)) {
		String_t S = tb_String("<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>\n");
		XmlElt_to_xml(S, XDoc(Doc)->root);
		return S;
	}
	return NULL;
}


inline int need_convert(char *s) {
	int convert = 0;
	while(*(s++)) { 
		if(! isascii(*s)) {
			convert = 1;
			break ;
		}
	}
	return convert;
}


static void startElement(void *userData, const char *name, const char **atts) {
	XmlDoc_t         Doc          = (XmlDoc_t) userData;
	Hash_t           Attr = tb_Hash();
	XmlElt_t         X;
	xml_tree_t       members = (xml_tree_t)Doc->members->instance;

	if(*atts) {
		String_t K, V;
		
		while(*atts ) {
			K = tb_String("%s", (char *)*atts++);
			V = tb_String("%s", (char *)*atts++);
			
			if(need_convert(tb_toStr(K))) tb_UTF8_to_Latin1(K);
			if(need_convert(tb_toStr(V))) tb_UTF8_to_Latin1(V);

			tb_Insert(Attr, V, tb_toStr(K));
			tb_Free(K);
		}
	} 
	X = tb_XmlElt(XELT_TYPE_NODE, members->cur, (char *)name, Attr);

	if(members->cur != NULL) {
		tb_Push(XXELT(members->cur)->Children, X);
	} else {
		members->root = X;
	}
	members->cur = X;
}


static void startCData(void *userData) {
	XmlDoc_t         Doc          = (XmlDoc_t) userData;
	xml_tree_t       m  = (xml_tree_t)Doc->members->instance;
	m->in_cdata = 1;
}

static void endCData(void *userData) {
	XmlDoc_t         Doc          = (XmlDoc_t) userData;
	xml_tree_t       m  = (xml_tree_t)Doc->members->instance;
	m->in_cdata = 0;
}

static void endElement(void *userData, const char *name) {
	XmlDoc_t Doc = (XmlDoc_t) userData;
	xml_tree_t       members = (xml_tree_t)Doc->members->instance;
	members->cur = XXELT(XXDOC(Doc)->cur)->Parent;
}

enum trimstates {lead,inside,trail};

String_t spacetrim(char *s) {
	int state = lead;
	char *start = s;
	char *end   = s;
	//	int flen = 0;
	do {
		if(isspace(*s)) {
			switch(state) 
				{
				case inside: 
					state = trail; 
					break;
				}
		} else {
			switch(state) 
				{
				case lead:
					state = inside;
					start = s;
					break;
				case trail:
					state = inside;
				}
			end = s;
		}
		//		flen++;
	} while(*s++);
	int len = (unsigned int)end - (unsigned int)start;
	if(len >0) {
		String_t S = tb_String(NULL);
		return tb_RawAdd(S, len, 0, start);
	}
	return NULL;
}




static void charElement(void *userData, const char *string, int len) {
	if(len >0) {
		XmlDoc_t Doc = (XmlDoc_t) userData;
		XmlElt_t X;
		String_t S = tb_String(NULL);
		xml_tree_t m = (xml_tree_t)Doc->members->instance;
		
		
		tb_RawAdd(S, len, -1, (char *)string);

#warning: may be not a great idea (forbids use of non latin1 charsets)
		if(need_convert(tb_toStr(S))) tb_UTF8_to_Latin1(S);
		
		if(XXELT(XXDOC(Doc)->cur)->xml_space == 0) { // squizz spaces

			String_t tmp = spacetrim(tb_toStr(S));
			if(tmp) {
				tb_Free(S);
				S = tmp;
			} else {
				tb_Free(S);
				return;
			}
		} // else  spaces are welcome
		
		if(XXDOC(Doc)->cur != NULL) {
			Vector_t Children    = XXELT(XXDOC(Doc)->cur)->Children;
			if(tb_getSize(Children ) > 0 ) {
				XmlElt_t lastSibling = tb_Get(Children, -1);
				if(m->in_cdata) {

					if(XELT_getType(lastSibling ) == XELT_TYPE_CDATA) {
						tb_RawAdd(XXELT(lastSibling)->Text, tb_getSize(S), -1, tb_toStr(S));
						tb_Free(S);
					} else {		
						X = tb_XmlElt(XELT_TYPE_CDATA, XXDOC(Doc)->cur, NULL, NULL);
						XXELT(X)->Text = S;
						TB_DOCK(XXELT(X)->Text);
						tb_Push(XXELT(XXDOC(Doc)->cur)->Children, X);
					}

				} else {

				if(XELT_getType(lastSibling ) == XELT_TYPE_TEXT) {
						tb_RawAdd(XXELT(lastSibling)->Text, tb_getSize(S), -1, tb_toStr(S));
					tb_Free(S);
				} else {		
					X = tb_XmlElt(XELT_TYPE_TEXT, XXDOC(Doc)->cur, NULL, NULL);
					XXELT(X)->Text = S;
					TB_DOCK(XXELT(X)->Text);
					tb_Push(XXELT(XXDOC(Doc)->cur)->Children, X);
				}
				}
			} else {		
				if(m->in_cdata) {
					X = tb_XmlElt(XELT_TYPE_CDATA, XXDOC(Doc)->cur, NULL, NULL);
					XXELT(X)->Text = S;
					TB_DOCK(XXELT(X)->Text);
					tb_Push(XXELT(XXDOC(Doc)->cur)->Children, X);
			} else {		
				X = tb_XmlElt(XELT_TYPE_TEXT, XXDOC(Doc)->cur, NULL, NULL);
				XXELT(X)->Text = S;
				TB_DOCK(XXELT(X)->Text);
				tb_Push(XXELT(XXDOC(Doc)->cur)->Children, X);
			}
			}

		} else {
			tb_error("Oops can't insert elm in null tree !\n");
		}
	} 
}





