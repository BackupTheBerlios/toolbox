/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: vector_iterators.c,v 1.1 2004/05/12 22:04:50 plg Exp $
//======================================================
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

#include <stdlib.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "Containers.h"
#include "Objects.h"
#include "iterators.h"
#include "Vector.h"
#include "Memory.h"
#include "Error.h"


// vector's implementation of interface "Iterable"

/* obsolete
#define XVIT(V)      ((vIter_t)((vector_extra_t)(V)->members->instance)->iter)

vIter_t v_getIterCtx(Vector_t V) { 
	if( XVIT(V) == NULL) {
		XVIT(V) = v_newIterCtx(V);
	}
	return XVIT(V);
}
*/

vIter_t v_newIterCtx(Vector_t V) {
	if( tb_getSize(V) >0) {
		vIter_t vi = (vIter_t)tb_xcalloc(1, sizeof(struct vIter));
		vi->V  = V;
		vi->cur_ndx = 0;
		return vi;
	}
	return NULL;
}

void v_freeIterCtx(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	if(vi != NULL) tb_xfree(vi);
}

retcode_t v_goFirst(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	if(vi == NULL) return TB_ERR;
	vi->cur_ndx = 0;
	return TB_OK;
}

retcode_t v_goLast(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	if(vi == NULL) return TB_ERR;
	vi->cur_ndx = XVector(vi->V)->size -1;
	return TB_OK;
}

tb_Object_t v_goNext(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	if(vi != NULL) {
		vector_members_t m = XVector(vi->V);
		vi->cur_ndx++;
		if( vi->cur_ndx >= 0 && vi->cur_ndx < m->size) {
			return (tb_Object_t) m->data[vi->cur_ndx];
		} 
		vi->cur_ndx = m->size -1;
	}
	return NULL;
}

tb_Object_t v_goPrev(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	if(vi != NULL) {
		vector_members_t m = XVector(vi->V);
		vi->cur_ndx--;
		if( vi->cur_ndx >= 0 && vi->cur_ndx < m->size) {
			return (tb_Object_t) m->data[vi->cur_ndx];
		} 
		vi->cur_ndx = 0;
	}
	return NULL;
}

tb_Key_t v_curKey(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	return (tb_Key_t)vi->cur_ndx;
}

tb_Object_t v_curVal(Iterator_t It) {
	vIter_t vi = (vIter_t)__getIterCtx(It);
	vector_members_t m = XVector(vi->V);
	if( vi->cur_ndx >= 0 && vi->cur_ndx < m->size) {
		return (tb_Object_t) m->data[vi->cur_ndx];
	} 
	return NULL;
}


