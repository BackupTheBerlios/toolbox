// ===============================================================================
// $Id: Objects.c,v 1.2 2004/05/14 15:22:38 plg Exp $
// ===============================================================================
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
 * @file Objects.c Toolbox Object base class definition
 */

/**
 * @defgroup Object Object_t methods
 * base class generic methods
 *
 * Toolbox base classes :
 * - TB_OBJECT
 *  - TB_SCALAR
 *   - TB_STRING       : dynamic string
 *   - TB_RAW          : binary data
 *   - TB_NUM          : integer
 *   - TB_FLOAT        : (NYI) real
 *   - TB_BOOL         : (NYI) boolean
 *   - TB_DATE         : datetime representation
 *   - TB_POINTER      : user data
 *  - TB_CONTAINER
 *   - TB_HASH         : hash tables
 *   - TB_VECTOR       : unidimensional dynamic arrays
 *   - TB_PROPERTY     : 
 *   - TB_DICT         : b+trees
 *  - TB_COMPOSITE
 *   - TB_SOCKET       : socket
 *   - TB_ITERATOR     : Container_t iterator
 *   - TB_XMLDOC       : xml document (dom alike)
 *   - TB_XMLELT       : xml element (dom alike)
 *
 * New classes can easily been derivated from Object_t (or any subclass) see classRegistry toolkit
 *
 * tb_Object_t object offers a 'virtual class' definition, referencing a set of generic methods and members that will be instanciated in derivated objects.
 * Among the members, is :
 * - alias refernce counter (object is not destroyed as long as ref counter is greater than 0)
 * - docking reference counter : incremented each time the object is inserted in a container ; and decremented each time it is removed. Object is not destroyed until this counter (and alias counter) reach 0.
 * - class type identifier
 * - members is a private data structure containing this instance own members, but also it's anscestors members, allowing full member inheritance. So it's possible for example to derivate a Vector to extends some sorting or filtering behaviours, and still use full Vectors interface for pushing, popping and accessing elements where applicable.
 * Class virtual methods allows to create, destroy, clone, dump, reset the objects.
 * @see Object, Scalar, Composite, Container
 */


#define __BUILD_OBJECTS

#include <pthread.h>
#include <stdlib.h>
#include "Objects.h"

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"

#include "classRegistry.h"
#include "tb_ClassBuilder.h"

#include <assert.h>
#include <stdio.h>


int OM_NEW;
int OM_FREE;
int OM_GETSIZE;
int	OM_CLONE;
int	OM_DUMP;
int	OM_CLEAR;
int	OM_INSPECT; // NYI -> to be used to allow self inspection and reflection
int	OM_STRINGIFY;


static tb_Object_t   tb_object_new   (void);
static void        * tb_object_free  (tb_Object_t O);
static void          tb_object_dump  (tb_Object_t O, int level);
char               * tb_object_stringify(tb_Object_t T);

inline const char   *tb_nameOf       (int Oid)    { return __class_name_of(Oid); }

inline retcode_t     TB_VALID        (tb_Object_t O, int toolbox_class_id) {
	if(O == NULL) return TB_KO;
	return __VALID_CLASS(O->isA, toolbox_class_id);
}

inline uint          tb_isA          (tb_Object_t O) { return (TB_VALID(O, TB_OBJECT)) ? O->isA : -1 ; }

/**
 * Retreive inherited members
 * \ingroup Object
 *
 * @see Object, tb_isA
*/
inline void *tb_getMembers(tb_Object_t This, int tb_type_id) {
	__members_t m = This->members;
	while(m) {
		if(m->instance_type == tb_type_id) return m;
		m=m->parent;
	}
	return NULL;
}

/**
 * Check class type
 * \ingroup Object
 *
 * tb_valid is a tb_isA companion function. It tests if an Object_t instance is really of type x, within inheritance : 
 * for example a TB_STRING is a :
 *  - TB_STRING (obviously)
 *  - TB_SCALAR (direct ancestor)
 *  - TB_OBJECT (root ancestor)
 *
 * example:
 * \code 
 * if(tb_valid(myObject, TB_CONTAINER, "my_function")) { ... }
 * \endcode
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if type mismatch
 * - TB_ERR_OBJECT_IS_NULL if myObject pointer is NULL
 * @see Object, tb_isA
*/
int tb_valid(void *O, int tb_type, char* func_name) {
	if(O) {
		int rc = TB_VALID(O, tb_type);
		if(! rc) {
			tb_error("%s: %p not a %s\n", func_name, O, __class_name_of(tb_type));
			set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		} else {
			no_error;
		}
		return rc;
	}
	tb_error("%s: pointer to object is NULL\n", func_name);
	set_tb_errno( TB_ERR_OBJECT_IS_NULL ); 
	return TB_KO;
}



void __build_object_once(int OID) {
	OM_NEW         = tb_registerNew_ClassMethod("New",          OID);
	OM_FREE        = tb_registerNew_ClassMethod("Free",         OID);
	OM_GETSIZE     = tb_registerNew_ClassMethod("getSize",      OID);
	OM_CLONE       = tb_registerNew_ClassMethod("Clone",        OID);
	OM_DUMP        = tb_registerNew_ClassMethod("Dump",         OID);
	OM_CLEAR       = tb_registerNew_ClassMethod("Clear",        OID);
	OM_INSPECT     = tb_registerNew_ClassMethod("Inspect",      OID);

	OM_STRINGIFY   = tb_registerNew_ClassMethod("Stringify",    OID);

	tb_registerMethod(OID, OM_NEW,  tb_object_new);
	tb_registerMethod(OID, OM_FREE, tb_object_free);
	tb_registerMethod(OID, OM_DUMP, tb_object_dump);
	tb_registerMethod(OID, OM_STRINGIFY, tb_object_stringify);
}


// find first (eventually overloaded) fnc of type Mid;
inline void *tb_getMethod(tb_Object_t O, int Mid) {
	void *p = __getMethod(tb_isA(O), Mid);
	if(p == NULL) {
		tb_error("tb_getMethod[%p]: no such method (%s)\n", O, __method_name_of(Mid));
		set_tb_errno(TB_ERR_NO_SUCH_METHOD);
	}
	return p;
}

inline void *tb_getMethodByName(tb_Object_t O, char *method_name) {
	return __getMethod(tb_isA(O), __method_id_of(method_name));
}

inline void *tb_getParentMethod(tb_Object_t O, int Mid) {
	return __getParentMethod(tb_isA(O), Mid);
}

inline void *tb_getParentMembers(tb_Object_t This) {
	return This->members->parent->instance;
}
/**
 * Unwind members stack 
 * 
 */
void tb_freeMembers(tb_Object_t This) {
	if(This && This->members) {
		void *m = This->members->parent;
		if(This->members->instance) {
			tb_xfree(This->members->instance);
		}
		tb_xfree(This->members);
		This->members = m;
	}
}

// [private] build object ancestor
tb_Object_t tb_newParent(int child) {
	void *p;
	tb_Object_t This;
	p = __getMethod( __parent_of(child), OM_NEW );
	if( p != NULL ) {
		__members_t new = (__members_t)tb_xcalloc(1, sizeof(struct __members));
		This = ((tb_Object_t (*)())p)();
		new->parent  = This->members;
		This->members = new;
		This->members->instance_type = child;
		return This;
	}
	tb_error("tb_newParent[%s] : no new() method\n", tb_nameOf(child));
	return NULL;
}

/**
 * Get size of Object_t.
 * \ingroup Object
 *
 * Size of a Object depends of is type.
 * A container returns number of elements, a String will returns a number
 * of chars, etc...
 * returns size of tb_Object or TB_ERR on error
 *
 * @param T Object_t target
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 * 
 * @see tb_Object_t
 * \todo should be object dependant (call tb_getMethod(T, OM_GETSIZE)() )
 */


// fixme: should be object dependant (call tb_getMethod(T, OM_GETSIZE)() )
int tb_getSize(tb_Object_t T) { 
	if(tb_valid(T, TB_OBJECT, __FUNCTION__)) {
		void *p;

		if((p = tb_getMethod(T, OM_GETSIZE))) {
			return ((int(*)(tb_Object_t))p)(T);
		} else {
			tb_error("%p (%d) [no getSize method]\n", T, T->isA);
			set_tb_errno(TB_ERR_NO_SUCH_METHOD);
		}
	}
	return -1;
}


// [private] Object constructor
tb_Object_t tb_object_new(void) {
	tb_Object_t This;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
  This = (tb_Object_t)tb_xcalloc(1, sizeof(struct tb_Object));
	This->refcnt = 1;
	This->members = (__members_t)tb_xcalloc(1, sizeof(struct __members));
	This->members->instance_type = TB_OBJECT;

	return This;
}

// [private] Object destructor
void *tb_object_free(tb_Object_t O) {
	if(tb_valid(O, TB_OBJECT, __FUNCTION__)) {
		O->isA = 0xdead;
		tb_xfree(O->members);
		tb_xfree(O);
	} 
	return NULL; // no more cascading destructors
}

/**
 * Duplicates a tb_Object
 * \ingroup Object
 *
 * Copy constructor. Source is untouched.
 * Object is copied in full depth (If clone source is a Container_t, all content is also copied). Socket_t are cloned in state TB_UNSET (connection is not realised)
 *
 * @return newly allocated clone (Object_t of same type than source).
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 

 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
 */
tb_Object_t tb_Clone(tb_Object_t T) {
	tb_Object_t Clone = NULL;
	void *p;

	if(! tb_valid(T, TB_OBJECT, __FUNCTION__)) return NULL;
 
	if((p = tb_getMethod(T, OM_CLONE)) != NULL) {
		Clone = ((tb_Object_t (*)(tb_Object_t))p)(T);
	} else {
		tb_error("%p (%d) [no clone method]\n", T, T->isA);
	}
	if(fm->dbg) fm_addObject(Clone);

  return Clone;
}

char *tb_object_stringify(tb_Object_t T) {
	return (char *)tb_nameOf(tb_isA(T));
}


/** 
 * Displays string representation of Object_t contents (when applicable)
 * \ingroup Object
 * Shows contents of tb_Object. A container will also display recursively his elements
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/
char *tb_Stringify(tb_Object_t O) {
	void *p;
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return NULL;

	if((p = tb_getMethod(O, OM_STRINGIFY))) {
		return ((char *(*)(tb_Object_t))p)(O); 
	} else {
		set_tb_errno(TB_ERR_NO_SUCH_METHOD);
		tb_error("%p (%d) [no dump method]\n", O, O->isA);
	}
	return NULL; // not likely to happen
}

/** 
 * Displays xml representation of Object_t internals.
 * \ingroup Object
 * Shows type and various infos related to tb_Object. A container will also display recursively his elements. Dump is redirected to stderr.
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/
void tb_Dump(tb_Object_t O) {
	void *p;
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return;

	if((p = tb_getMethod(O, OM_DUMP))) {
		((void (*)(tb_Object_t, int))p)(O, 0); 
	} else {
		set_tb_errno(TB_ERR_NO_SUCH_METHOD);
		tb_error("%p (%d) [no dump method]\n", O, O->isA);
	}
}


/**
 * Create an alias for a tb_Object.
 * \ingroup Object
 * Increments the internal reference counter for object.
 * A subsequent free will first decrements ref counter, allowing many threads to deal with same shared instance, freeing it anytime with no need to synchronize with others.
 *
 * @param O the target object
 * @return Object_t alias (points to same instance than O)
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/
tb_Object_t tb_Alias(tb_Object_t O) {
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return NULL;

  O->refcnt++;
  return O;
}

/**
 * Reset a tb_Objet
 * Clearing a tb_Object depends of is type. A tb_Container will be emptied, a tb_String set to "", and a tb_Socket disconnected.
 *
 *
 * @param O the target object
 * @return target Object_t after cleanse
 *
 * \remarks in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/
tb_Object_t tb_Clear(tb_Object_t O) {
	void *p;

	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return O;

	if((p = tb_getMethod(O, OM_CLEAR))){
    return ((tb_Object_t(*)(tb_Object_t))p)(O);
  } else {
		tb_error("%p (%d) [no clear method]\n", O, O->isA);
		set_tb_errno(TB_ERR_NO_SUCH_METHOD);
  }
	return O;
}




// [private] Object_t internal dump method
void tb_object_dump( tb_Object_t O, int level ) {
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return;

	fprintf(stderr, "<TB_OBJECT type=\"%d\" addr=\"%p\"/>\n",
					O->isA, O);
}
	






#ifdef tb_Free
#undef tb_Free
#endif

 


void dbg_tb_free(char *func, char *file, int line, void *mem) {
	set_tb_mdbg(func, file, line);
	return tb_Free(mem);
}
 
/** 
 * Free tb_Object.
 * \ingroup Object
 *
 * Free all memory allocated for this Object_t instance.
 *
 * Objects are really freed only if reference counter is less or equal to 1, and if docked counter is 0. To free a docked Object_t, it must first be extracted from it's parent Container_t (or you can use tb_Remove). Docked means 'inside a container' (inserted in a Hash_t, a Vector_t ...).
 * If the Object to be freed is a container, every inner elements will be freed also. If reference counter is greater than 0 (aliased instance, see tb_Alias), it will be decremented but not yet freed.
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 *
 * @see tb_Object_t 
 * @see tb_Free, tb_Clear, tb_Clone, tb_Alias, tb_isA, tb_Dump, tb_getSize, tb_toStr, tb_toInt , tb_Marshall, tb_unMarshall
*/
void tb_Free(tb_Object_t O) {
	void *p;

	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) return;

	if( O->refcnt >1 && O->docked >0 ) {
		// object is docked : don't dec refcnt lower than 1
		O->refcnt --;
	} else if( O->refcnt >0 && O->docked == 0) {
		// object is not docked : I can lower refcnt to 0
		O->refcnt --;
	}

	if( O->docked == 0 ) {
		if( O->refcnt == 0) {

			p = tb_getMethod(O, OM_FREE);
			if(fm->dbg) fm_delObject(O);

			// destructor calls ancestor's one up to innermost level : tb_Object_t
			while( p ) {
				p = ((void *(*)(tb_Object_t))p)(O); 
			}
		} else { 
			tb_info("tb_Free: Obj's <%p> refcnt > 0 (%d) , not destroyed.\n", O, O->refcnt); 
		}

	} else { 
		tb_info("tb_Free: Obj's <%p> still docked %d times , not destroyed.\n", O, O->docked); 
	}
}


