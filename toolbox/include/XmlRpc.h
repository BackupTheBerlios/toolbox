/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: XmlRpc.h,v 1.1 2004/05/26 14:39:17 plg Exp $
//======================================================

// created on Tue May 11 23:37:45 2004

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

/*
<Rpc methodName="myFunc">
  [<[paramIn|paramOut|paramInOut] 
	       type="[int|boolean|string|double|dateTime.iso8601|base64|array|struct"/>]+
</Rpc>
*/

// Class XmlRpc public methods

#ifndef __XMLRPC_H
#define __XMLRPC_H

#include "Toolbox.h"
#include "Composites.h"

typedef tb_Object_t           XmlRpc_t;

extern int XMLRPC_T;

// --[public methods goes here ]--

// constructors
XmlRpc_t XmlRpc();
XmlRpc_t XmlRpc_new();
// factories (produce new object(s))
/*...*/
// manipulators (change self) 
//retcode_t XRpc_setsignatures(XmlRpc_t T, Hash_t val);

/*...*/
// inspectors (don't change self) 
//Hash_t XRpc_getsignatures(XmlRpc_t T);

/*...*/
enum Xrpc_argstyle {
	 XMLRPC_PARAM_IN,
	 XMLRPC_PARAM_INOUT,
	 XMLRPC_PARAM_OUT,
};


enum Xrp_errors {
	XMLRPC_FAULT_TYPE_MISMATCH    = -3,
	XMLRPC_FAULT_BAD_RESPONSE     = -4,
	XMLRPC_FAULT_NOT_RESPONDING   = -5,
	XMLRPC_FAULT_NO_SUCH_CALL     = -6,
};

retcode_t   XRpc_registerRemoteMethod    (XmlRpc_t This, char *signature);
retcode_t   XRpc_registerLocalMethod     (XmlRpc_t This, char *signature, void *function);
void *      XRpc_getMethod               (XmlRpc_t This, void *methodName);


XmlDoc_t    XRpc_getSignature            (XmlRpc_t This, char *methodName);
String_t    XRpc_getMethodName           (XmlElt_t RootElt);
String_t    XRpc_getQueryMethodName      (XmlElt_t RootElt);
int         XRpc_getParamType            (XmlElt_t Elt);
int         XRpc_getResponseParamType    (XmlElt_t Elt);
int         XRpc_getParamStyle           (XmlElt_t Elt);
retcode_t   XRpc_unMarshallAndMerge      (tb_Object_t Dst, XmlElt_t Elt);

void        XRpc_receiveCall             (Socket_t So);
int         XRpc_sendCall(XmlRpc_t Xr, /*Uri_t Dest*/ Socket_t So, char *func, ...);
#endif

