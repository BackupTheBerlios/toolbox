/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: XmlRpc_impl.c,v 1.1 2004/05/12 22:04:53 plg Exp $
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

// Class XmlRpc (extends TB_COMPOSITE, [implements interface ...])
// description :


/* uncomment only for internal Toolbox Class 
#ifndef BUILD
#define BUILD
#endif
*/

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "XmlRpc.h"
#include "XmlRpc_impl.h"
#include "tb_ClassBuilder.h"

int XMLRPC_T;

static void *XmlRpc_free( XmlRpc_t Obj);
//static  XmlRpc_t XmlRpc_clone(XmlRpc_t Obj);
//static  void XmlRpc_dump(XmlRpc_t Obj);
//static  XmlRpc_t XmlRpc_clear(XmlRpc_t Obj);
//...

void setup_XmlRpc_once(int OID);

inline XmlRpc_members_t XXmlRpc(XmlRpc_t T) {
	return (XmlRpc_members_t)((__members_t)tb_getMembers(T, XMLRPC_T))->instance;
}

pthread_once_t __init_XmlRpc_once = PTHREAD_ONCE_INIT;
void init_XmlRpc_once() {
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	// if you doesn't extends directly from a toolbox class, you must call your anscestor init
	XMLRPC_T = tb_registerNewClass("XMLRPC_T", TB_COMPOSITE, setup_XmlRpc_once);
}

void setup_XmlRpc_once(int OID) {
	/* OM_NEW and OM_FREE are mandatory methods */
	tb_registerMethod(OID, OM_NEW,                    XmlRpc_new);
	tb_registerMethod(OID, OM_FREE,                   XmlRpc_free);
	/*  others are optionnal: this ones are common and shown as example */
//	tb_registerMethod(OID, OM_CLONE,                  XmlRpc_clone);
//	tb_registerMethod(OID, OM_DUMP,                   XmlRpc_dump);
//	tb_registerMethod(OID, OM_CLEAR,                  XmlRpc_clear);
}


XmlRpc_t dbg_XmlRpc(char *func, char *file, int line) {
	set_tb_mdbg(func, file, line);
	return XmlRpc_new();
}


 /** XmlRpc_t constructor
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
 * \ingroup XMLRPC
 */

XmlRpc_t XmlRpc() {
	return XmlRpc_new();
}



XmlRpc_t XmlRpc_new() {
	tb_Object_t This;
	pthread_once(&__init_XmlRpc_once, init_XmlRpc_once);
	XmlRpc_members_t m;
	This =  tb_newParent(XMLRPC_T); 
	
	This->isA  = XMLRPC_T;



	m = (XmlRpc_members_t)tb_xcalloc(1, sizeof(struct XmlRpc_members));
	This->members->instance = m;

	m->signatures = tb_Hash();
	TB_DOCK(m->signatures);

	if(fm->dbg) fm_addObject(This);

	return This;
}

void *XmlRpc_free(XmlRpc_t Obj) {
	if(tb_valid(Obj, XMLRPC_T, __FUNCTION__)) {
		XmlRpc_members_t m = (XmlRpc_members_t)XXmlRpc(Obj);
		fm_fastfree_on();
		
		
		TB_UNDOCK(m->signatures);
		tb_Free(m->signatures);
	
		tb_freeMembers(Obj);
		fm_fastfree_off();
		Obj->isA = XMLRPC_T; // requiered for introspection (as we are unwinding dtors stack)
    return tb_getParentMethod(Obj, OM_FREE);
	}

  return NULL;
}

/*
static XmlRpc_t XmlRpc_clone(XmlRpc_t This) {
}
*/

/*
static void XmlRpc_dump(XmlRpc_t This) {
}
*/



