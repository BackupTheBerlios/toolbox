//------------------------------------------------------------------
// $Id: Vector.h,v 1.2 2004/05/19 15:13:54 plg Exp $
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


#ifndef __TB_VECTOR_H
#define __TB_VECTOR_H

#include "Toolbox.h"
#include "Containers.h"

#define VECTOR_DEFAULT_START 10
#define VECTOR_DEFAULT_STEP  10

struct vIter {
	Vector_t V;
	int cur_ndx;
};
typedef struct vIter *vIter_t;

struct vector_members {
	tb_Object_t  * data;
	String_t       Stringified;
	int            size;
  int            nb_slots;
  int            frozen;
	int            flag;
	int            start_sz;
	int            step_sz;
};
typedef struct vector_members *vector_members_t;

inline vector_members_t XVector(Vector_t);

vIter_t            v_getIterCtx       (Vector_t H);
vIter_t            v_newIterCtx       (Vector_t H);
void               v_freeIterCtx      (Vector_t H);
retcode_t          v_goFirst          (Iterator_t It);
retcode_t          v_goLast           (Iterator_t It);
tb_Object_t        v_goNext           (Iterator_t It);
tb_Object_t        v_goPrev           (Iterator_t It);
tb_Key_t           v_curKey           (Iterator_t It);
tb_Object_t        v_curVal           (Iterator_t It);



#endif
 
