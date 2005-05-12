//------------------------------------------------------------------
// $Id: Pointer.h,v 1.3 2005/05/12 21:54:36 plg Exp $
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

#ifndef __HAVE_TB_POINTER_H
#define __HAVE_TB_POINTER_H

#include "Toolbox.h"

void * tb_pointer_free     (Pointer_t S);
void   tb_pointer_dump     (Pointer_t P, int level);
int    tb_pointer_getsize  (Pointer_t P);

Pointer_t Pointer_new();
Pointer_t Pointer_clear(Pointer_t Self);
Pointer_t Pointer_ctor(Pointer_t Self, void *p, void *free_fnc);

struct pointer_members {
	void *userData;
	void (*freeUserData)(void *);
	int  size;
};
typedef struct pointer_members *pointer_members_t;
inline pointer_members_t XPtr(Pointer_t);

#endif
