/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Iterable_interface.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//======================================================

// created on Thu Aug  1 15:23:25 2002

/* Copyright (c) 1999-2004, Paul L. Gatille. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 *
 *
 * You should have received a copy of the Artistic License with this Kit,
 * in the file named "Artistic License". If not, I'll be glad to provide one.
 */


// interface Iterable : object content can be accessed iteratively

#ifndef __ITERABLE_INTERFACE_H
#define __ITERABLE_INTERFACE_H

#include <pthread.h>

extern int IFACE_ITERABLE;
extern int OM_NEW_ITERATOR_CTX;
extern int OM_FREE_ITERATOR_CTX;
extern int OM_GET_ITERATOR_CTX;
extern int OM_GONEXT;
extern int OM_GOPREV;
extern int OM_FIND;
extern int OM_GOFIRST;
extern int OM_GOLAST;
extern int OM_CURKEY;
extern int OM_CURVAL;
extern pthread_once_t __iterable_build_once;
void build_iterable_once();

#endif
