/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: C_Castable_interface.c,v 1.1 2004/05/12 22:04:52 plg Exp $
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


// interface C-Castable : object type can be cast into lagacy C type


#include <pthread.h>

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Objects.h"
#include "tb_ClassBuilder.h"
#include "C_Castable_interface.h"


int IFACE_C_CASTABLE;
int OM_TOSTRING;
int OM_TOINT;
int OM_TOPTR;
pthread_once_t __c_castable_build_once = PTHREAD_ONCE_INIT;   

void build_c_castable_once() {
	IFACE_C_CASTABLE = tb_registerNewInterface("C_Castable");

	OM_TOSTRING    = tb_registerNew_InterfaceMethod("toStr", IFACE_C_CASTABLE);
	OM_TOINT       = tb_registerNew_InterfaceMethod("toInt", IFACE_C_CASTABLE);
	OM_TOPTR       = tb_registerNew_InterfaceMethod("toPtr", IFACE_C_CASTABLE);
}


/**
 * Sends back Object_t representation into legacy C string (char *).
 * \ingroup Object
 * Object_t target may be either a scalar (no other arg is required) or a Container_t. Later case require a second arg of type char * if obj is a Hash_t or int for Vector_t
 * @return string if possible, else NULL
 *
 *
 * \warning tb_toStr is provided as a convenient accessor (or typecaster) from Object_t to char *. Returned char * must be dealt as _read-only_ value. Direct access to core data is not garanted in next versions. Never use to gain access to internal data and mess into (or worse : to free it). You would have been foretold.
 *
 * in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 *
 * @param O the target object or container
 * @param ... if object is a Container_t, the accessor (char *key or int ndx) to the target
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 * - TB_ERR_NO_SUCH_METHOD if obj can't be stringified
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_Share, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/

char *tb_toStr(tb_Object_t O, ...) {
	tb_Key_t key;
	va_list parms;
	void *p = NULL;
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return NULL;
	if(TB_VALID(O, TB_CONTAINER)) { // intentionnaly using MACRO TB_VALID, not tb_valid
		va_start(parms, O);
		key = va_arg(parms, tb_Key_t);
		O = tb_Get(O, key);
	}

	p = tb_getMethod(O, OM_TOSTRING);
	if( p != NULL) {
		return ((char * (*)(tb_Object_t))p)(O);		
	}
	set_tb_errno(TB_ERR_NO_SUCH_METHOD);
	
	return NULL;
}

/**
 * Convert a Object_t into a int 
 * \ingroup Object
 * \warning doesn't make sense for other type than String_t or Num_t 
 *
 * @return converted int or 0
 *
 * in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 *
 * @param O the target object or container
 * @param ... if object is a Container_t, the accessor (char *key or int ndx) to the target
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 * - TB_ERR_NO_SUCH_METHOD if obj can't be stringified
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_Share, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/

int tb_toInt(tb_Object_t O, ...) {
	tb_Key_t key;
	va_list parms;
	void *p;
	no_error;
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return TB_ERR;
	
	if(TB_VALID(O, TB_CONTAINER)) { // intentionnaly using MACRO TB_VALID, not tb_valid
		va_start(parms, O);
		key = va_arg(parms, tb_Key_t);
		O = tb_Get(O, key);
	}

	if(O && (p = tb_getMethod(O, OM_TOINT))) {
		return ((int (*)(tb_Object_t))p)(O);		
	}
	set_tb_errno(TB_ERR_NO_SUCH_METHOD);

	return TB_ERR;
}
