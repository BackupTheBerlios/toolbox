/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: C_Castable_interface.h,v 1.2 2004/07/01 21:37:01 plg Exp $
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


// interface C-Castable : object can be cast (transtyped) into lagacy C type
#ifndef __C_CASTABLE_INTERFACE_H
#define __C_CASTABLE_INTERFACE_H

#include <pthread.h>

extern int IFACE_C_CASTABLE;
extern int OM_TOSTRING;
extern int OM_TOINT;
extern int OM_TOPTR;
extern pthread_once_t __c_castable_build_once;   
void build_c_castable_once();
#endif
