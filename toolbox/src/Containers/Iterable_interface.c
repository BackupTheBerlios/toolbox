/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Iterable_interface.c,v 1.1 2004/05/12 22:04:50 plg Exp $
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


// interface Iterable : object content can be accessed iteratively


#include <pthread.h>

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Objects.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"


int IFACE_ITERABLE;
int OM_NEW_ITERATOR_CTX;
int OM_FREE_ITERATOR_CTX;
int OM_GET_ITERATOR_CTX;
int OM_GONEXT;
int OM_GOPREV;
int OM_GOFIRST;
int OM_FIND;
int OM_GOLAST;
int OM_CURKEY;
int OM_CURVAL;
pthread_once_t __iterable_build_once = PTHREAD_ONCE_INIT;   

void build_iterable_once() {

	IFACE_ITERABLE        = tb_registerNewInterface("Iterable");

	OM_NEW_ITERATOR_CTX   = tb_registerNew_InterfaceMethod("newIterCtx",   IFACE_ITERABLE);
	OM_FREE_ITERATOR_CTX  = tb_registerNew_InterfaceMethod("freeIterCtx",  IFACE_ITERABLE);
	OM_GET_ITERATOR_CTX   = tb_registerNew_InterfaceMethod("getIterCtx",   IFACE_ITERABLE);
	OM_GONEXT             = tb_registerNew_InterfaceMethod("goNext",       IFACE_ITERABLE);
	OM_GOPREV             = tb_registerNew_InterfaceMethod("goPrev",       IFACE_ITERABLE);
	OM_GOFIRST            = tb_registerNew_InterfaceMethod("goFirst",      IFACE_ITERABLE);
	OM_FIND               = tb_registerNew_InterfaceMethod("Find",         IFACE_ITERABLE);
	OM_GOLAST             = tb_registerNew_InterfaceMethod("goLast",       IFACE_ITERABLE);
	OM_CURKEY             = tb_registerNew_InterfaceMethod("curKey",       IFACE_ITERABLE);
	OM_CURVAL             = tb_registerNew_InterfaceMethod("curVal",       IFACE_ITERABLE);
}

