//======================================================
// $Id: Vector.c,v 1.3 2004/05/24 16:37:52 plg Exp $
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

/**
 * @file Vector.c Vector_t object definition
 */

/** @defgroup Vector Vector_t 
 * @ingroup Container
 * Vector specific related functions
 */
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "Vector.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"
#include "Serialisable_interface.h"

#include "Memory.h"
#include "Error.h"

inline vector_members_t XVector(Vector_t V) {
	return (vector_members_t )((__members_t)V->members)->instance;
}

static void        tb_vector_dump         (Vector_t V, int level);
static Vector_t    tb_vector_new          ();
static void      * tb_vector_free         (tb_Object_t O);
static Vector_t    tb_vector_clone        (Vector_t V);
static tb_Object_t tb_vector_clear        (Vector_t V);
static int         tb_vector_getsize      (Vector_t V);
static tb_Object_t tb_vector_get          (Vector_t V, int ndx);
static tb_Object_t tb_vector_take         (Vector_t V, int ndx);
static retcode_t   tb_vector_replace      (Vector_t V, tb_Object_t data, int ndx);
static retcode_t   tb_vector_insert       (Vector_t V, tb_Object_t data, int ndx);
static retcode_t   tb_vector_remove       (Vector_t V, int ndx) ;

static String_t    tb_vector_stringify   (Vector_t V);


static void        tb_vector_marshall     (String_t marshalled, Vector_t V, int level);
static Vector_t    tb_vector_unmarshall   (XmlElt_t xml_element);

static int vector_replace(Vector_t V, tb_Object_t O, int ndx);
static int vector_swap   (Vector_t V, int ndx1, int ndx2);

void __build_vector_once(int OID) {
	tb_registerMethod(OID, OM_NEW,                    tb_vector_new);
	tb_registerMethod(OID, OM_FREE,                   tb_vector_free);
	tb_registerMethod(OID, OM_GETSIZE,                tb_vector_getsize);
	tb_registerMethod(OID, OM_CLONE,                  tb_vector_clone);
	tb_registerMethod(OID, OM_DUMP,                   tb_vector_dump);
	tb_registerMethod(OID, OM_CLEAR,                  tb_vector_clear);
	tb_registerMethod(OID, OM_REPLACE,                tb_vector_replace);
	tb_registerMethod(OID, OM_INSERT,                 tb_vector_insert);
	tb_registerMethod(OID, OM_TAKE,                   tb_vector_take);
	tb_registerMethod(OID, OM_GET,                    tb_vector_get);
	tb_registerMethod(OID, OM_REMOVE,                 tb_vector_remove);

	tb_registerMethod(OID, OM_NEW_ITERATOR_CTX,       v_newIterCtx);
	tb_registerMethod(OID, OM_FREE_ITERATOR_CTX,      v_freeIterCtx);
	tb_registerMethod(OID, OM_GONEXT,                 v_goNext);
	tb_registerMethod(OID, OM_GOPREV,                 v_goPrev);
	tb_registerMethod(OID, OM_GOFIRST,                v_goFirst);
	tb_registerMethod(OID, OM_GOLAST,                 v_goLast);
	tb_registerMethod(OID, OM_CURKEY,                 v_curKey);
	tb_registerMethod(OID, OM_CURVAL,                 v_curVal);

	tb_registerMethod(OID, OM_STRINGIFY,              tb_vector_stringify);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

	tb_registerMethod(OID, OM_MARSHALL,     tb_vector_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_vector_unmarshall);

}

// ======================================================================
// Vectors
// ======================================================================


Vector_t dbg_tb_vector(char *func, char *file, int line) {
	set_tb_mdbg(func, file, line);
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	return tb_vector_new();
}



/** Vector_t contructor.
 * \ingroup Vector
 * Allocates and initialise a new Vector_t. 
 * Vector_t is a dynamic structure that can be accessed either as an conventional array (by indices) or as a stack (pop/push, shift/unshift). Growth and shrink are dynamic. Bound checking is operated on indiced access. 
 *
 * @return newly allocated Vector_t
 *
 * Specific Methods:
 * - tb_Push    : add Object_t on tail
 * - tb_Pop     : extract and returns last Object_t
 * - tb_Unshift : add Object_t on head
 * - tb_Shift   : extract and returns first Object_t
 * - tb_toArgv  : create argv-like struct from Vector_t contents
 * - tb_Merge   : insert Vector_t contents into another Vector_t
 * - tb_Splice  : extract Vector_t slice into another Vector_t
 * - tb_Sort    : sort Vector_t
 * - tb_Reverse : reverse Vector_t content ordering
 *
 * @see Container, tb_Push, tb_Pop, tb_Unshift, tb_Shift, tb_toArgv, tb_Merge, tb_Splice, tb_Sort, tb_Reverse
 * \todo tb_fromArgv (construct a Vector from argv)
*/
Vector_t tb_Vector() {
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	return tb_vector_new();
}

static int tb_vector_getsize(Vector_t V) {
	return XVector(V)->size;
}

static Vector_t tb_vector_new() {
	Vector_t This;
	vector_members_t m;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	This = tb_newParent(TB_VECTOR);

	This->isA   = TB_VECTOR;
	This->members->instance = (vector_members_t)tb_xcalloc(1, sizeof(struct vector_members));
	m = This->members->instance;

	m->start_sz = m->nb_slots = VECTOR_DEFAULT_START;
	m->step_sz  = VECTOR_DEFAULT_STEP;
  m->data = tb_xcalloc(1, (m->nb_slots * sizeof(tb_Object_t))); 

	if(fm->dbg) fm_addObject(This);

	return This;
}

static void *tb_vector_free(Vector_t V) {
	int i;
	vector_members_t m = XVector(V);
	fm_fastfree_on();
	for(i = 0; i <m->size; i++) {
		TB_UNDOCK(m->data[i]);
		tb_Free(m->data[i]);
	}
	tb_xfree(m->data);
	tb_freeMembers(V);
	fm_fastfree_off();

	return tb_getParentMethod(V, OM_FREE);
}


static retcode_t tb_vector_insert(Vector_t V, tb_Object_t data, int ndx) {
	vector_members_t m = XVector(V);

	if(ndx < 0) ndx = m->size - TB_ABS(ndx);
	if(ndx > m->size) {
		tb_error("tb_vector_insert: unbound ndx (%d) sz=%d",
						 ndx, m->size); 
		set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
		return TB_ERR;
	}

	m->size++;
	if(m->size > m->nb_slots) {
		m->nb_slots += m->step_sz;
		m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t)));
		assert(m->data != NULL);
	}
	memmove(m->data + (ndx + 1) * sizeof(tb_Object_t),
					m->data + ndx * sizeof(tb_Object_t),
					sizeof(tb_Object_t));
	m->data[ndx] = data;
	TB_DOCK(data);

	return TB_OK;
}

static retcode_t tb_vector_replace(Vector_t V, tb_Object_t data, int ndx) {
	tb_Object_t O;
	vector_members_t m = XVector(V);

	if(ndx < 0) ndx = m->size - TB_ABS(ndx);
	if(!(ndx < m->size)) {
		tb_error("tb_vector_add: unbound ndx (%d) sz=%d",
						 ndx, m->size); 
		set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
		return TB_ERR;
	}

	if((O = m->data[ndx]) != NULL) {
		TB_UNDOCK(O);
		tb_Free(O);
	}
	m->data[ndx] = data;
	TB_DOCK(data);

	return TB_OK;
}




/** Add an Object_t after Vector_t's last element.
 * \ingroup Vector
 *
 * Appends a reference of Object_t target into Vector_t.
 * @param V Vector_t target
 * @param data Object to be stored
 * @return retcode_t
 * \warning No copy done. Target 'data' Object_t reference is copyied in last position of Vector_t, enlarging it if necessary. Object_t docking counter is incremented and further freeing attempts on this Object_t will be uneffectives as long it remains docked.  
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V is not a TB_VECTOR
 * - TB_ERR_OBJECT_IS_NULL if data is not a TB_OBJECT
 *
 * @see Container, Vector, tb_Pop
*/
retcode_t tb_Push(Vector_t V, tb_Object_t data) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__) &&
		 tb_valid(data, TB_OBJECT, __FUNCTION__)) {
		vector_members_t m = XVector(V);

		m->size++;
		if(m->size > m->nb_slots) {
			m->nb_slots += m->step_sz;
			m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t)));
		}

		m->data[m->size -1] = data;
		TB_DOCK(data);

		return TB_OK;
	}
	return TB_ERR;
}


/** Extract last Vector_t's element.
 * \ingroup Vector
 *
 * Extract and returns last Vector_t's element.
 * \warning No copy done. Target 'data' Object_t reference is removed from Vector_t, shrinking it if necessary. Object_t docking counter is decremented accordingly.
 * Object_t is \em not freed (see tb_Remove).
 * @param V target Vector_t
 * @return last target element or NULL if empty
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * - TB_ERR_EMPTY_VALUE if V is empty
 * @see Container, Vector, tb_Push, tb_Remove
*/
tb_Object_t tb_Pop(Vector_t V) { 
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		tb_Object_t obj;
		vector_members_t m = XVector(V);

		if(m->size == 0) {
			set_tb_errno(TB_ERR_EMPTY_VALUE);
			return NULL;
		}
		obj = m->data[m->size -1]; 
		TB_UNDOCK(obj);
		m->size--;
		if((m->nb_slots - m->size) >= (m->step_sz *2)) {
			m->nb_slots = m->size + m->step_sz;
			m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t))); 
		}

		return obj;
	}
	return NULL;
}

/** Add an Object_t before Vector_t's first element.
 * \ingroup Vector
 *
 * Prepends a reference of Object_t target into Vector_t.
 * \warning No copy done. Target 'data' Object_t reference is copyied in last position of Vector_t, enlarging it if necessary. Object_t docking counter is incremented and further freeing attempts on this Object_t will be uneffectives as long it remains docked (See tb_Remove)
 *
 * @param V target Vector_t
 * @param data object to be stored
 * @returns retcode_t
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V is not a TB_VECTOR
 * - TB_ERR_OBJECT_IS_NULL if data is not a TB_OBJECT
 * @see Container, Vector, tb_Shift
 */
retcode_t tb_Unshift(Vector_t V, tb_Object_t data) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__) &&
		 tb_valid(data, TB_OBJECT, __FUNCTION__)) {
		vector_members_t m = XVector(V);
	
		m->size++;

		if(m->size > m->nb_slots) {
			m->nb_slots += m->step_sz;
			m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t)));
		}
	
		memmove(m->data +1, m->data, 
						((m->size - 1) * sizeof(tb_Object_t)));

		m->data[0] = data;
		TB_DOCK(data);

		return TB_OK;
	}
	return TB_ERR;
}

/** Extract first Vector_t's element.
 * \ingroup Vector
 *
 * Extract and returns first Vector_t's element.
 * \warning No copy done. Target 'data' Object_t reference is removed from Vector_t, shrinking it if necessary. Object_t docking counter is decremented accordingly. Object_t is \em not freed (see tb_Remove).
 *
 * @param V target Vector_t
 * @return last target element or NULL if empty
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * - TB_ERR_EMPTY_VALUE if V is empty
 * @see Container, Vector, tb_unShift
 */
tb_Object_t tb_Shift(Vector_t V) { 
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		tb_Object_t obj;
		vector_members_t m = XVector(V);

		if(m->size == 0) {
			set_tb_errno(TB_ERR_EMPTY_VALUE);
			return NULL;
		}
		obj = m->data[0];
		TB_UNDOCK(obj);
		m->size--;
		memmove(m->data, m->data +1, (m->size * sizeof(tb_Object_t)));
		if((m->nb_slots - m->size) >= (m->step_sz *2)) {
			m->nb_slots = m->size + m->step_sz;
			m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t))); 
		}

		return obj;
	}
	return NULL;
}


static tb_Object_t tb_vector_take(Vector_t V, int ndx) {
	tb_Object_t obj = NULL;
	vector_members_t m = XVector(V);
	
	if(ndx < 0) ndx = m->size - TB_ABS(ndx);
	if(!(ndx < m->size)) {
		tb_error("tb_vector_take: unbound ndx (%d) sz=%d",
						 ndx, m->size); 
		set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
		return NULL;
	}

	obj = m->data[ndx];
	TB_UNDOCK(obj);
	m->size--;
	memmove(m->data + ndx, m->data + ndx +1, 
					((m->size - ndx) * sizeof(tb_Object_t)));
	if((m->nb_slots - m->size) >= (m->step_sz *2)) {
		m->nb_slots = m->size + m->step_sz;
		m->data = tb_xrealloc(m->data, (m->nb_slots * sizeof(tb_Object_t))); 
	}

	return obj;
}

static retcode_t tb_vector_remove(Vector_t V, int ndx) {
	tb_Object_t obj;
	vector_members_t m = XVector(V);

	if(ndx < 0) ndx = m->size - TB_ABS(ndx);
	if(!(ndx < m->size)) {
		tb_error("tb_vector_remove(%p): unbound ndx (%d) sz=%d",
						 V, ndx, m->size); 
		set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
		return TB_ERR;
	}

	if((obj = tb_vector_take(V, ndx))) {
		tb_Free(obj);
	}
	return TB_OK;
}


static tb_Object_t tb_vector_get(Vector_t V, int ndx) {
	tb_Object_t rez = NULL;
	vector_members_t m = XVector(V);

	if(ndx < 0) ndx = m->size - TB_ABS(ndx);
	if(!(ndx < m->size)) {
		tb_error("tb_vector_get(%p): unbound ndx (%d) sz=%d",
						 V, ndx, m->size); 
		set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
		return NULL;
	}

	if(ndx < m->size && ndx >= 0) rez = m->data[ndx];

	return rez;
}


static tb_Object_t tb_vector_clear(Vector_t V) {
	int i;
	vector_members_t m = XVector(V);
	
	if(m->size == 0) return V;

	fm_fastfree_on();
	for(i = 0; i < m->size; i++){
		TB_UNDOCK(m->data[i]);
		tb_Free(m->data[i]);
	}
	tb_xfree(m->data);
	fm_fastfree_off();
	m->data = tb_xcalloc(1, (m->start_sz * sizeof(tb_Object_t))); 
	m->nb_slots = m->start_sz;
	m->size = 0;

	return V;
}

static Vector_t tb_vector_clone(Vector_t V) {
	int i; 
	Vector_t clone;
	vector_members_t m = XVector(V);

	clone = tb_Vector();
	for(i = 0; i < m->size; i++) {
		tb_Push(clone, tb_Clone(m->data[i]));
	}

	return clone;
}

/** Copy a Vector of tb_Strings into an argv structure.
 * \ingroup Vector
 *
 * Create a legacy C argv type char ** NULL terminated, from a Vector.
 * \warning Only String_t elements are added.
 * @param V target Vector_t
 * @return char ** (argv like) or NULL if error
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * @see Container, Vector, tb_freeArgv
 */
char **tb_toArgv(Vector_t V) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		char **argv;
		int i, sz, nb = 0;

		sz = tb_getSize(V);

		argv	= (char **)tb_xcalloc(1, sizeof(char *) * (sz +1));
		for(i = 0; i < sz; i++) {
			if(tb_valid(tb_Get(V, (tb_Key_t)i), TB_STRING, __FUNCTION__)) {
				nb++;
				argv[i] = tb_xstrdup((char *)S2sz(tb_Get(V, (tb_Key_t)i)));
			}
		}
		if(nb < sz) argv = tb_xrealloc(argv, sizeof(char *) * (nb +1));

		return argv;
	}
	return NULL;
}


/** Free a tb_toArgv result.
 * \ingroup Vector
 *
 * \warning must be a _real_ argv (allocated vector of char*, null terminated) or beware to sigsegv !
 * @see Container, Vector, tb_toArgv
*/
void tb_freeArgv(char **argv) {
	char **s = argv;
	while(*s) tb_xfree(*s++);
	tb_xfree(argv);
}

static void tb_vector_dump(Vector_t V, int level) {
	int i, sz;
	vector_members_t m = XVector(V);

	sz = m->size;
	for(i = 0; i<level; i++) fprintf(stderr, " ");
	if(m->size == 0) {
		fprintf(stderr, 
						"<TB_VECTOR SIZE=\"%d\" ADDR=\"%p\" DATA=\"%p\" REFCNT=\"%d\" DOCKED=\"%d\"/>\n",
						sz, V, m->data, V->refcnt, V->docked);
	} else {
		fprintf(stderr, 
						"<TB_VECTOR SIZE=\"%d\" ADDR=\"%p\" DATA=\"%p\" REFCNT=\"%d\" DOCKED=\"%d\">\n",
						sz, V, m->data, V->refcnt, V->docked);
		for(i = 0; i <sz; i++) {
			void *p = tb_getMethod(m->data[i], OM_DUMP);
			if(p) ((void(*)(tb_Object_t,int))p)(m->data[i] ,level+1);
			else {
				for(i = 0; i<level; i++) fprintf(stderr, " ");
				fprintf(stderr, "<TB_EMPTY/>\n");
			}
		}
	}
	for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, "</TB_VECTOR>\n");
}




/** Insert Vector_t's content into another.
 * \ingroup Vector
 *
 * Appends into destination Vector_t, begining at 'start' offset all elements of source Vector_t. Offset may be negative to address element starting by the end of the vector (-1 is last element). 'How' param define the type of operation.
 *
 * @param dst : Vector_t destination of the operation
 * @param src : Vector_t source 
 * @param start : offset of starting element in source container
 * @param how :
 * (choose one in set, not OR-able)
 * - TB_MRG_MOVE : source elements are removed from source containers before appending to destination
 * - TB_MRG_COPY : source elements are copied from source to destination (see tb_Clone)
 * - TB_MRG_ALIAS : source elements are aliased from source to destination (see tb_Alias)
 *
 * @return destination Vector_t
 *
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if dst or src are not of TB_VECTOR type
 * - TB_ERR_OUT_OF_BOUNDS if 'start' is out of bounds
 * @see Container, Vector, tb_Clone, tb_Alias
 */
Vector_t tb_Merge(Vector_t dst, Vector_t src, int start, int how) {

	no_error;

	if(tb_valid(dst, TB_VECTOR, __FUNCTION__) &&
		 tb_valid(src, TB_VECTOR, __FUNCTION__)) {
		int i, sz, old_sz;;
		vector_members_t ms = XVector(src);
		vector_members_t md = XVector(dst);

		if(start < 0) start = md->size - TB_ABS(start) +1;
		if(!(start <= md->size)) {
			tb_error("tb_Merge (%p): unbound start (%d) sz=%d",
							 dst, start, md->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return NULL;
		}
		sz = ms->size;
		old_sz = md->size;
		md->size +=sz;

		if(md->size > md->nb_slots) {
			md->nb_slots += (TB_MAX((md->step_sz), sz));
			md->data = tb_xrealloc(md->data, (md->nb_slots * sizeof(tb_Object_t)));
		}
		memmove(md->data + (start + sz) * sizeof(tb_Object_t),
						md->data + start * sizeof(tb_Object_t),
						(old_sz - start) * sizeof(tb_Object_t));
		for(i=0; i<sz; i++) {
			tb_Object_t T = NULL;
			switch(how) {
			case TB_MRG_COPY:
				md->data[start + i] = T = tb_Clone(tb_Get(src, i));
				break;
			case TB_MRG_MOVE:
			case TB_MRG_ALIAS:
			default:
				md->data[start + i] = T = tb_Alias(tb_Get(src, i));
				break;
			}
			TB_DOCK(T);
		}
		if(how == TB_MRG_MOVE) tb_Clear(src);

		return dst;
	}
	return NULL;
}


/** Create a new Vector_t from another Vector_t part.
 * \ingroup Vector
 *
 * Allocate a new Vector_t, then appends 'len' elements form target Vector_t, starting at 'start' offset. Type of operation is set by 'how'. Offset may be negative to address element starting by the end of the vector (-1 is last element). Length of '-1' mean 'all remaining elements'.
 *
 * @param V : source Vector_t
 * @param start : offset of begining in source container
 * @param len : number of element
 * @param how : (choose one in set, not OR-able)
 * - TB_MRG_MOVE : source elements are removed from source containers before appending to destination
 * - TB_MRG_COPY : source elements are copied from source to destination (see tb_Clone)
 * - TB_MRG_ALIAS : source elements are aliased from source to destination (see tb_Alias)
 *
 * @return new built Vector_t or NULL if error
 *
 * \remarks
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * - TB_ERR_OUT_OF_BOUNDS if 'start' out of bounds
 * @see Container, Vector, tb_Clone, tb_Alias
 */
Vector_t tb_Splice(Vector_t V, int start, int len, int how) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		int i;
		Vector_t W;
		vector_members_t mW, m = XVector(V);
	
		if(len == -1) {
			if(start == -1) {
				len = m->size;
				start = 0;
			} else {
				len = (m->size - TB_ABS(start));
			}
		}
		else {
			len = TB_ABS(len);
		}

		if(start < 0) start = m->size - (TB_ABS(start) +len) +1;

		if(start > m->size) {
			tb_error("tb_vector_splice: unbound ndx (%d) sz=%d",
							 start, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return NULL;
		}
		if(start+len > m->size) {
			tb_error("tb_vector_splice: unbound len (%d) sz=%d",
							 len, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return NULL;
	
		}

		W = tb_Vector();
		mW = XVector(W);

		mW->data = tb_xrealloc(mW->data, (len)* sizeof(tb_Object_t));
		tb_notice("tb_vector_splice: start:%d len %d", start, len);
		for(i=0; i<len; i++) {
			tb_Object_t T = NULL;
			switch(how) {
			case TB_MRG_COPY:
				mW->data[i] = T = tb_Clone(tb_Get(V, start+i));
				break;
			case TB_MRG_MOVE:
				mW->data[i] = T = tb_Take(V, start+i);
				start--;
				break;
			case TB_MRG_ALIAS:
			default:
				mW->data[i] = T = tb_Alias(tb_Get(V, start+i));
				break;
			}
			TB_DOCK(T);
		}
		mW->size += len;

		return W;
	}
	return NULL;
}

/** Get Object_t's index in Vector_t.
 * \ingroup Vector
 * @param V target's container
 * @param O target object
 * @return index of O in V if found else -1
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V is not a TB_VECTOR
 * @see Container, Vector
*/
int tb_getIdByRef(Vector_t V, tb_Object_t O) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		int i;
		vector_members_t m = XVector(V);

		for(i=0; i<m->size; i++) {
			if(m->data[i] == O) return i;
		}
	}
	return TB_ERR;
}

/** Sort Vector_t contents.
 *
 * Sort is done using user comparaison function. If NULL, a default func is provided. Default comp function is meaningfull on String_t only. If you need to sort other kind of Object_t, you must provide your own.
 * @param V : target Vector_t to be sorted
 * @param cmp : user comparaison function, or NULL to default to string compare
 * @return sorted target Vector_t
 *
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * @see Container, tb_Vector, tb_Reverse
*/
Vector_t tb_Sort(Vector_t V, int (*cmp)(tb_Object_t, tb_Object_t)) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		int h = 1;
		int N;
		int i, j;
		tb_Object_t O;
		vector_members_t m = XVector(V);

		N = m->size;

		if(cmp == NULL) cmp = tb_default_cmp;

		while(h < N) h = 3*h +1;

		do {
			h /= 3;
			for(i = h; i < N; ++i) {
				if(cmp((O = tb_Get(V, (tb_Key_t)i)), tb_Get(V, (tb_Key_t)(i-h))) <= 0) {
					j = i;
					do {
						vector_replace(V, tb_Get(V, (tb_Key_t)(j-h)), j);
						j = j - h;
					} while(j >= h && cmp(tb_Get(V, (tb_Key_t)(j-h)), O) > 0);
					vector_replace(V, O, j);
				}
			}
		} while(h > 1);

		return V;
	}
	return NULL;
}


static int vector_replace(Vector_t V, tb_Object_t O, int ndx) {
	vector_members_t m = XVector(V);
	if(ndx > m->size) return TB_KO;
	m->data[ndx] = O;
	return TB_OK;
}

int tb_default_cmp(tb_Object_t A, tb_Object_t B) {
	char *a, *b;
	// fixme: if a || b not string, cmp on size
	if(tb_isA(A) != TB_STRING) a = "";
	else a = S2sz(A);
	if(tb_isA(B) != TB_STRING) b = "";
	else b = S2sz(B);

	return(strcmp(a, b));
}

int vector_swap(Vector_t V, int ndx1, int ndx2) {
	vector_members_t m = XVector(V);
	tb_Object_t O = m->data[ndx1];
	if(ndx1 <0 || ndx1 >m->size) return TB_KO;
	if(ndx2 <0 || ndx2 >m->size) return TB_KO;

	m->data[ndx1] = m->data[ndx2];
	m->data[ndx2] = O;
	return TB_OK;
}
 


/** Reverse Vector_t contents ordering.
 * \ingroup Vector
 *
 * All elements are swapped into Vector_t.
 *
 * @param V target Vector_t
 * @return reversed target Vector_t
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * @see Container, Vector, tb_Sort
 */
Vector_t tb_Reverse(Vector_t V) {
	no_error;
	if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
		int i = 0, j = XVector(V)->size -1;

		while(i < j) {
			vector_swap(V, i, j);
			i++;
			j--;
		}
		return V;
	}
	return NULL;
}



static String_t tb_vector_stringify(Vector_t V) {
	int i, sz;
	vector_members_t m = XVector(V);
	String_t str = tb_String(NULL);
	sz = m->size;
	if(m->size == 0) {
		tb_StrAdd(str, -1,	"()");
	} else {
		tb_StrAdd(str, -1,	"(");
		for(i = 0; i<sz; i++) {
			void *p = tb_getMethod(m->data[i], OM_STRINGIFY);
			if(p) {
				String_t Rez =  ((String_t(*)(tb_Object_t))p)(m->data[i]);
				tb_StrAdd(str, -1,	"%S", Rez);
				tb_Free(Rez);
				if(i<sz-1) {
					tb_StrAdd(str, -1,	", ");
				}
			}
		}
		tb_StrAdd(str, -1, ")");
	}
	return str;
}



static void tb_vector_marshall(String_t marshalled, Vector_t V, int level) {
	int i, sz;
	char indent[level+1];
	vector_members_t m = XVector(V);

	
	if(marshalled == NULL) return;
	memset(indent, ' ', level);
	indent[level] = 0;

	sz = m->size;
	if(m->size == 0) {
		tb_StrAdd(marshalled, -1,	"%s<array>\n%s  <data/></value>%s</array>\n", indent, indent, indent);
	} else {
		tb_StrAdd(marshalled, -1,	"%s<array>\n%s  <data>\n", indent, indent);
		for(i = 0; i <sz; i++) {
			void *p = tb_getMethod(m->data[i], OM_MARSHALL);
			if(p) {
				tb_StrAdd(marshalled, -1,	"%s  <value>\n", indent, indent);
				((void(*)(String_t, tb_Object_t, int))p)(marshalled, m->data[i], level+6);
				tb_StrAdd(marshalled, -1,	"%s  </value>\n", indent, indent);
			} 
		}
		tb_StrAdd(marshalled, -1, "%s  </data>\n%s</array>\n", indent, indent);
	}
}

static Vector_t tb_vector_unmarshall(XmlElt_t xml_element) {
	Vector_t V, Ch;
	int children, i;
 
	if(! streq(S2sz(XELT_getName(xml_element)), "array")) {
		tb_error("tb_vector_unmarshall: not a vector Elmt\n");
		return NULL;
	}
	V = tb_Vector();
	// FIXME: *very* buggy design mr UserLand ...
	Ch = XELT_getChildren(tb_Get(XELT_getChildren(xml_element), 0));

	children = tb_getSize(Ch);
	tb_info("in vector unmarshaller (%d childs)\n", children);

	for(i = 0; i<children; i++) {
		// even worse :
		tb_Object_t O = tb_XmlunMarshall(tb_Get(XELT_getChildren(tb_Get(Ch, i)),0));
		if(O) {
			tb_Push(V, O);
		} else {
			tb_warn("unmarshalling pb\n");
		}
	}

	return V;

}

















