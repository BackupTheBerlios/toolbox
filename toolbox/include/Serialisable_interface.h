/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Serialisable_interface.h,v 1.1 2004/05/12 22:04:48 plg Exp $
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
// mainly useful for presistent storage and remote IPCs. 
// Current implementation use xmlrpc conventions (see xmlrpc.org).
#ifndef __SERIALISABLE_INTERFACE_H
#define __SERIALISABLE_INTERFACE_H

#include <pthread.h>

extern int IFACE_SERIALISABLE;
extern int OM_MARSHALL;
extern int OM_UNMARSHALL;
extern pthread_once_t __serialisable_build_once;   
void build_serialisable_once();
#endif
