/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: XmlRpc.c,v 1.2 2004/05/24 16:37:53 plg Exp $
//======================================================

// created on Tue May 11 23:37:45 2004 by Paul Gatille <paul.gatille\@free.fr>

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
 * @file XmlRpc.c
 */

/**
 * @defgroup Xmlrpc XmlRpc_t
 * @ingroup Composite
 * Class XmlRpc, support XmlRpc seralisation/deserialisation (see full specs at http://www.xmlrpc.com/)
 */

#ifndef __BUILD
#define __BUILD
#endif

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "XmlRpc.h"
#include "XmlRpc_impl.h"
#include "tb_ClassBuilder.h"

#include "Raw.h"
/* 

	specific public class and related methods goes here


retcode_t XRpc_setsignatures(XmlRpc_t T, Hash_t val) {
}


Hash_t XRpc_getsignatures(XmlRpc_t T) {
}


*/


/**
 * Register Remote RPC (to be used in client-side)
 *
 * Use this function from the RPC client to register a remote function, and it's signature.
 * 
 * Signature format is (this isn't a DTD ;) :
 * \code
 * <XmlRpc name="myFunc">
 * [<[paramIn|paramOut|paramInOut] type="int"/> ]...
 * </XmlRpc>
 * \endcode
 * Params elements are either paramIn, paramOut, or paramInOut. Only 'Out' and 'InOut' parameters are sent back to caller.
 * Param type is one of :
 * - int
 * - bool   [NYI]
 * - double [NYI]
 * - string
 * - date
 * - array (vector of any xmlrpc type, including arrays and structs)
 * - struct (pairs of name, value where value is any xmlrpc type, including array and structs)
 *
 * All those types are mapped to Toolbox classes : 
 * - int     -> Num_t
 * - bool    -> [NYI]
 * - double  -> [NYI]
 * - string  -> String_t
 * - date    -> Date_t
 * - array   -> Vector_t
 * - struct  -> Hash_t
 * 
 * @param This : Target storing object
 * @param signature : xml string abiding to upper syntax
 * @return retcode_t
 * @see XRpc_sendCall, XRpc_receiveCall
 * @ingroup Xmlrpc
 */
retcode_t   XRpc_registerRemoteMethod    (XmlRpc_t This, char *signature) {
	if(tb_valid(This, XMLRPC_T, __FUNCTION__)) {
		XmlDoc_t Doc = tb_XmlDoc(signature);
		// fixme: need to validate
		String_t methodName = XELT_getAttribute(XDOC_getRoot(Doc), "methodName");
		if(methodName != NULL) {
			tb_Insert(XXmlRpc(This)->signatures, Doc, tb_toStr(methodName));
		}
		return TB_OK;
	}
	return TB_KO;
}

/**
 * Register Local RPC (to be used in server-side)
 *
 * Use this function from the RPC server to register a local function, and it's signature.
 * 
 * Signature format is (this isn't a DTD ;) :
 * \code
 * <XmlRpc name="myFunc">
 * [<[paramIn|paramOut|paramInOut] type="int"/> ]...
 * </XmlRpc>
 * \endcode
 * Params elements are either paramIn, paramOut, or paramInOut. Only 'Out' and 'InOut' parameters are sent back to caller.
 *
 * @param This : Target storing object
 * @param signature : xml string abiding to upper syntax
 * @param function : native function to be called. Obviously this function should exists and use the same signature (number and types of arguments) as declared in second parameter.
 * @return retcode_t
 *
 * @see XRpc_sendCall, XRpc_receiveCall
 * @ingroup Xmlrpc
 */
retcode_t   XRpc_registerLocalMethod     (XmlRpc_t This, char *signature, void *function) {
	if(tb_valid(This, XMLRPC_T, __FUNCTION__)) {
		XmlDoc_t Doc = tb_XmlDoc(signature);
		// fixme: need to validate
		String_t methodName = XELT_getAttribute(XDOC_getRoot(Doc), "methodName");
		tb_Insert(XELT_getAttributes(XDOC_getRoot(Doc)), tb_Pointer(function, NULL), "methodPointer");
		if(methodName != NULL) {
			tb_Insert(XXmlRpc(This)->signatures, Doc, tb_toStr(methodName));
		}
		return TB_OK;
	}
	return TB_KO;
}

XmlDoc_t   XRpc_getSignature            (XmlRpc_t This, char *methodName) {
	if(tb_valid(This, XMLRPC_T, __FUNCTION__)) {
		return tb_Get(XXmlRpc(This)->signatures, methodName);
	}
	return NULL;
}

/*
<XmlRpc name="myFunc">
  <paramIn name="size" type="int"/>
	<paramInOut name="state" type="string"/>
</XmlRpc>
*/
String_t    XRpc_getMethodName(XmlElt_t RootElt) {
	return XELT_getAttribute(RootElt, "methodName");
}

/*
<?xml version="1.0"?>
<methodCall>
   <methodName>myFunc</methodName>
   <params>
      <param>
         <value><int>41</int></value>
      </param>
      <param>
         <value><string>voila</string></value>
      </param>
   </params>
</methodCall>
*/
String_t XRpc_getQueryMethodName(XmlElt_t RootElt) {
	return XELT_getText(tb_Get(XELT_getChildren(tb_Get(XELT_getChildren(RootElt),0)),0)); // ouf !
}

void * XRpc_getMethod(XmlRpc_t This, void *methodName) {
	XmlDoc_t Sig = XRpc_getSignature(This, methodName);
	XmlElt_t Xe  = XDOC_getRoot(Sig);
	
	return P2p(XELT_getAttribute(Xe, "methodPointer"));
}


int         XRpc_getParamType            (XmlElt_t Elt) {
	//<param[In|InOut|Out] type="t"/>
	String_t Type = XELT_getAttribute(Elt, "type");
	if(tb_StrEQ(Type,        "string")) {
		return TB_STRING;
	} else if(tb_StrEQ(Type, "int") ||
						tb_StrEQ(Type, "i4") ) {
		return TB_NUM;
		//	} else if(tb_StrEQ(XELT_getName(Xe), "boolean")) {
		//	} else if(tb_StrEQ(XELT_getName(Xe), "double")) {
		//	} else if(tb_StrEQ(XELT_getName(Xe), "dateTime.iso8601")) {
	} else if(tb_StrEQ(Type, "base64")) {
		return TB_RAW;
	} else if(tb_StrEQ(Type, "struct")) {
		return TB_VECTOR;
	} else if(tb_StrEQ(Type, "array")) {
		return TB_HASH;
	} 
	return -1;
}

/*
<?xml version="1.0"?>
<methodResponse>
   <params>
      <param>
         <value><string>South Dakota</string></value>
      </param>
   </params>
</methodResponse>
*/

int XRpc_getResponseParamType(XmlElt_t Elt) {
	/*
      <param>
         <value><string>South Dakota</string></value>
      </param>
	 */
	String_t Type = XELT_getName(Elt);
	if(tb_StrEQ(Type,        "string")) {
		return TB_STRING;
	} else if(tb_StrEQ(Type, "int") ||
						tb_StrEQ(Type, "i4") ) {
		return TB_NUM;
		//	} else if(tb_StrEQ(XELT_getName(Xe), "boolean")) {
		//	} else if(tb_StrEQ(XELT_getName(Xe), "double")) {
		//	} else if(tb_StrEQ(XELT_getName(Xe), "dateTime.iso8601")) {
	} else if(tb_StrEQ(Type, "base64")) {
		return TB_RAW;
	} else if(tb_StrEQ(Type, "struct")) {
		return TB_VECTOR;
	} else if(tb_StrEQ(Type, "array")) {
		return TB_HASH;
	} 
	return -1;
}


int         XRpc_getParamStyle           (XmlElt_t Elt) {
	//  <paramIn name="size" type="int"/>
	if(tb_StrEQ(XELT_getName(Elt), "paramIn")) {
		return XMLRPC_PARAM_IN;
	} else if(tb_StrEQ(XELT_getName(Elt), "paramOut")) {
		return XMLRPC_PARAM_OUT;
	} else if(tb_StrEQ(XELT_getName(Elt), "paramInOut")) {
		return XMLRPC_PARAM_INOUT;
	}
	return -1;
}


retcode_t    XRpc_unMarshallAndMerge      (tb_Object_t Target, XmlElt_t Elt) {
	int ret = TB_KO;
	switch(tb_isA(Target)) {
	case TB_STRING:
		{
			Vector_t V;
			if((V = XELT_getChildren(Elt)) != NULL &&
				 tb_getSize(V) >0) {
				tb_Clear(Target);
				tb_StrAdd(tb_Clear(Target), -1, "%S", XELT_getText(tb_Get(V, 0)));
				ret = TB_OK;
			}
		}
		break;
	case TB_NUM:
		{
			Vector_t V;
			if((V = XELT_getChildren(Elt)) != NULL &&
				 tb_getSize(V) >0) {
				tb_NumSet(Target, tb_toInt(XELT_getText(tb_Get(V, 0))));
				ret = TB_OK;
			}
		}
		break;
	case TB_RAW:
		{
			Vector_t V;
			if((V = XELT_getChildren(Elt)) != NULL &&
				 tb_getSize(V) >0) {
				raw_members_t m = XRaw(Target);
				tb_Clear(Target);
				m->size = tb_DecodeBase64(tb_toStr(XELT_getText(tb_Get(V, 0))), 
																						 &(m->data));
				ret = TB_OK;
			}
		}
		break;
		/*
	case TB_DOUBLE:
	case TB_DATE:
	case TB_BOOL:
		//NYI
		break;
		*/
	case TB_VECTOR:
		{
			Vector_t V;
			if((V = XELT_getChildren(Elt)) != NULL &&
				 tb_getSize(V) >0) {
				Vector_t New = tb_unMarshall(Elt);
				tb_Clear(Target);
				tb_Merge(Target, New, 0, TB_MRG_MOVE);
				tb_Free(New);
				ret = TB_OK;
			}
		}
		break;
	case TB_HASH:
		{
			Vector_t V;
			if((V = XELT_getChildren(Elt)) != NULL &&
				 tb_getSize(V) >0) {
				Hash_t New = tb_unMarshall(Elt);
				Iterator_t It = tb_Iterator(New);
				if(It) {
					tb_Clear(Target);
					do {
						tb_Insert(Target, tb_Alias(tb_Value(It)), K2s(tb_Key(It)));
					} while(tb_Next(It));
					tb_Free(It);
					tb_Free(New);
				}
				ret = TB_OK;
			}
		}
		break;
	}

	return ret;
}













