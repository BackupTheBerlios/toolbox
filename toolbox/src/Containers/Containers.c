//===========================================================
// $Id: Containers.c,v 1.1 2004/05/12 22:04:50 plg Exp $
//===========================================================
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
 * @file Containers.c Containers type methods
 */

/**
 * @defgroup Container Containers types
 * Objects collections abstractions
 *
 * Containers objects are collections abstractions, able to store any Toolbox type (including other Container_t). This allows fast and efficient building of dynamic structures in a very 'Perl-ish' way. 
 * Basically, objects are \em not copyied (duplicated) when inserted into the container, but instead their 'docking counter' is incremented. This docking counter is a reference counter that will protect the coherency of a Container_t by insuring that a referenced object is not freed.
 * Many generic methods (applying to all Container_t types) can be used to insert, remove, access or iterate the content of Containers.
 *
 * @see Container, Iterator, Hash, Vector, Dict
 * @see tb_Insert, tb_Replace, tb_Get, tb_Remove, tb_Take, tb_Exists
 * @ingroup Object
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "Toolbox.h"
#include "tb_global.h"
#include "Objects.h"
#include "Containers.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"
#include "Memory.h"
#include "Error.h"


int OM_REPLACE;
int	OM_INSERT;
int OM_EXISTS;
int OM_TAKE;
int OM_GET;
int OM_REMOVE;


void __build_container_once(int OID) {
	OM_REPLACE             = tb_registerNew_ClassMethod("Replace",          OID);
	OM_INSERT              = tb_registerNew_ClassMethod("Insert",           OID);
	OM_TAKE                = tb_registerNew_ClassMethod("Take",             OID);
	OM_EXISTS              = tb_registerNew_ClassMethod("Exists",           OID);
	OM_GET                 = tb_registerNew_ClassMethod("Get" ,             OID);
	OM_REMOVE              = tb_registerNew_ClassMethod("Remove",           OID);

	tb_implementsInterface(OID, "Iterable", 
												 &__iterable_build_once, build_iterable_once);

	tb_registerMethod(OID, OM_NEW,          tb_container_new);
	tb_registerMethod(OID, OM_FREE,         tb_container_free);
	tb_registerMethod(OID, OM_DUMP,         tb_container_dump);
}

tb_Object_t tb_container_new() {
	tb_Object_t O;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	O = tb_newParent(TB_CONTAINER);
	O->isA = TB_CONTAINER;
	return O;
}

void *tb_container_free(Container_t C) {
	if(tb_valid(C, TB_CONTAINER, __FUNCTION__)) {
		C->isA = TB_CONTAINER;
		tb_freeMembers(C);
		return tb_getParentMethod(C, OM_FREE);
	}
	return NULL;
}


void tb_container_dump(tb_Object_t O, int level) {
	int i;
	no_error;
	if(! tb_valid(O, TB_CONTAINER, __FUNCTION__))	return;
	
	for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, "<%s TYPE=\"%d\" REFCNT=\"%d\" DOCKED=\"%d\" ADDR=\"%p\" />\n",
					tb_nameOf(O->isA), O->isA, O->refcnt, O->docked, O);
}


/**
* Add/overwrites an Object_t into a Container_t.
* \ingroup Container
* Object_t 'O' is added into Container 'Cn', using the second parameter as index.
*
* As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
* -  char * key if container is a Hash_t or Dict_t
* -  int key if container is a Vector_t
*
* Example:
* \code
* Hash_t H  = tb_Hash();
* Vector_t V = tb_Vector();
* 
* tb_Replace(V, tb_String(NULL), 0);
* \endcode
*
* \warning 
* - in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
* - For both containers tb_Replace overwrites old element if exists for 'key'
*
* @param Cn : destination container
* @param O : replacement tb_Object_t
* @param ... : key/ndx accessor into Cn
* @return retcode_t
* \remarks ERROR:
* in case of error tb_errno will be set to :
* - TB_ERR_INVALID_OBJECT if obj not a Container_t
* - TB_ERR_BAD if invalid Object_t obj
* - TB_ERR_OUT_OF_BOUNDS if index out of Vector_t bounds
* @see Container, Iterator, Hash, Vector, Dict
* @see tb_Insert, tb_Replace, tb_Get, tb_Remove, tb_Take, tb_Exists
*/
retcode_t    tb_Replace(Container_t Cn, tb_Object_t O, ...) {
	tb_Key_t key;
	void *p;
	va_list parms;
	no_error;
	if(! tb_valid(Cn, TB_CONTAINER, __FUNCTION__)) { 
		tb_error("tb_Replace: '1st arg not a container\n");
		return TB_ERR;
	}
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) { 
		tb_error("tb_Replace: 2nd arg not a tb_object\n");
		return TB_ERR;
	}

	va_start(parms, O);
	key = va_arg(parms, tb_Key_t);
	if((p = tb_getMethod(Cn, OM_REPLACE))) {
		return ((retcode_t (*)(Container_t,tb_Object_t,tb_Key_t))p) (Cn, O, key);
	}

	return TB_KO;
}


/** Insert a tb_Object_t to a Container_t
 *
 * tb_Object_t 'O' is inserted into Container_t 'Cn'
 * 
 * As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
 * -  char * key if container is a Hash_t
 * -  int key if container is a Vector_t
 *
 * tb_Insert always add Object_t in Vector_t (provided that index fit into bounds), enlarging
 * Container_t as necessary ; but will insert into Hash_t only if no prior occurence of key is found
 *
 * Example :
 * \code
 * Hash_t H  = tb_Hash();
 * Vector_t V = tb_Vector();
 *
 * // if "titi" key exists, insert won't be done
 * tb_Insert(H, tb_String(NULL), "titi");
 * // if V[1] exists, insert will be done, and prior contant will be shifted
 * tb_Insert(V, tb_String(NULL), 1);
 * \endcode
 *
 * \warning in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 *
 * @param C : target Container_t
 * @param O : tb_Object_t to be inserted
 * @param ... : accessor to dock tb_Object_t
 * @return retcode_t
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a Container_t
 * - TB_ERR_BAD if invalid Object_t obj
 * - TB_ERR_ALLREADY if key already set in Hash_t
 * - TB_ERR_OUT_OF_BOUNDS if index out of Vector_t bounds
 * @see Container, Iterator, Hash, Vector, Dict
 * @see tb_Exists, tb_Insert, tb_Replace, tb_Get, tb_Take, tb_Remove
 * @ingroup Container
 */
retcode_t    tb_Insert(Container_t C, tb_Object_t O, ...) {
	tb_Key_t key;
	void *p;
	va_list parms;
	no_error;
	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__)) { 
		tb_error("tb_Insert: 1st arg not a container\n");
		return TB_ERR;
	}
	if(! tb_valid(O, TB_OBJECT, __FUNCTION__)) { 
		tb_error("tb_Insert: 2nd arg not a tb_object\n");
		return TB_ERR;
	}

	va_start(parms, O);
	key = va_arg(parms, tb_Key_t);
	if((p = tb_getMethod(C, OM_INSERT))) {
		return ((retcode_t    (*)(Container_t,tb_Object_t,tb_Key_t))p) (C, O, key);
	}

	return TB_KO;
}


/** Search container for key accessor.
 *
 * As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
 * -  char * key if container is a Hash_t or Dict_t
 * -  int key if container is a Vector_t
 *
 * \warning in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 *
 * @param C : target Container_t
 * @param ... : accessor to docked tb_Object_t
 * @return retcode_t (TB_OK if found, TB_KO if not found, TB_ERR on error)
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if obj not a TB_OBJECT
 * @see Container, Iterator, Hash, Vector, Dict
 * @see tb_Exists, tb_Insert, tb_Replace, tb_Get, tb_Take, tb_Remove
 * @ingroup Container
*/
retcode_t    tb_Exists(Container_t C, ...) {
	tb_Key_t key;
	void *p;
	va_list parms;
	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__)) return TB_ERR;

	va_start(parms, C);
	key = va_arg(parms, tb_Key_t);

	if((p = tb_getMethod(C, OM_EXISTS))) {
		return ((int(*)(Container_t, tb_Key_t))p)(C, key); 
	}

	return TB_KO;
}


/** Accessor to a docked tb_Object_t (a tb_Object_t into a Container_t).
 * tb_Get is useful to gain access to a object stocked inside a container when you don't want to extract it. 
 *
 * As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
 * -  char * key if container is a Hash_t or Dict_t
 * -  int key if container is a Vector_t
 *
 * \warning in case of a Vector_t, index must respect allocated bounds. but you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 * \warning The reference returned is to be considered as \em read-only. As the Object is still docked, it can't be freed. See tb_Remove, or tb_Get if you want to destroy it.
 *
 * @param C : target Container_t
 * @param ... : accessor to docked tb_Object_t
 * @return reference on found Object_t or NULL
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if C not a TB_CONTAINER
 * @see Container, Iterator, Hash, Vector, Dict
 * @see tb_Exists, tb_Insert, tb_Replace, tb_Get, tb_Take, tb_Remove
 * @ingroup Container
*/
tb_Object_t tb_Get(Container_t C, ...) {
	tb_Key_t key;
	va_list parms;
	void *p;
	no_error;
	
	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__))	return NULL;
	
	va_start(parms, C);
	key = va_arg(parms, tb_Key_t);

	if((p = tb_getMethod(C, OM_GET))) {
		return ((tb_Object_t(*)(Container_t, tb_Key_t))p)(C, key); 
	}

	return NULL;
}




/** Extract a docked Object_t (a tb_Object_t into a Container_t).
 *
 * As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
 * -  char * key if container is a Hash_t or Dict_t
 * -  int key if container is a Vector_t
 *
 * Decrements Object's docking counter, and destroy its reference from Container_t
 * 
 * \warning 
 * - tb_Take returns the Object, you must then take care of it, and destroy it when no more in use, in opposition to tb_Get which only give 'a view' of the docked tb_Object_t .  
 * - in case of a Vector_t, index must respect allocated bounds. but
 * you can use negatives offsets to address element starting by the end of the array (-1 is last element)
 *
 * @param C : target Container_t
 * @param ... : accessor to docked tb_Object_t
 * @return found Object_t or NULL on error
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if C not a TB_CONTAINER
 * @see Container, Iterator, Hash, Vector, Dict
 * @see tb_Exists, tb_Insert, tb_Replace, tb_Get, tb_Take, tb_Remove
 * @ingroup Container
*/
tb_Object_t tb_Take(Container_t C, ...) {
	tb_Key_t key;
	va_list parms;
	void *p;
	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__))	return NULL;
	
	va_start(parms, C);
	key = va_arg(parms, tb_Key_t);

	if((p = tb_getMethod(C, OM_TAKE))) {
		return ((tb_Object_t(*)(tb_Object_t, tb_Key_t))p)(C, key); 
	}

	return NULL;
}

retcode_t dbg_tb_remove(char *func, char *file, int line, Container_t C, ...) {
	tb_Key_t key;
	va_list parms;
	set_tb_mdbg(func, file, line);

	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__))	return TB_ERR;
	
	va_start(parms, C);
	key = va_arg(parms, tb_Key_t);

	return tb_Remove(C, key);
}

/** Extract and destroy a tb_Object_t from a Container_t.
 *
 * Object identified by second argument is removed, it's docking counter is 
 * decremented, and freed if possible (if no Alias or docked reference remains).
 *
 * As for every Container_t methods, a key is needed as last parameter. This parameter depends of Container_t type :
 * -  char * key if container is a Hash_t or Dict_t
 * -  int key if container is a Vector_t
 * 
 * Object reference designed by 'key' is destroyed from 'Obj' Container_t
 * 
 * Example :
 * \code
 * Hash_t H  = tb_Hash();
 * Vector_t V = tb_Vector();
 * 
 * ...
 * 
 * tb_Remove(H, "titi");
 * tb_Remove(V, 0);
 *
 * \endcode
 * \warning in case of a Vector_t, index must respect allocated bounds. but 
you can use negatives offsets to address element starting by the end of the array (-1 is last element)
* @return  retcode_t
* \remarks ERROR:
* in case of error tb_errno will be set to :
* - TB_ERR_INVALID_OBJECT if C not a TB_CONTAINER
* @see Container, Iterator, Hash, Vector, Dict
* @see tb_Exists, tb_Insert, tb_Replace, tb_Get, tb_Take, tb_Remove
* @ingroup Container
*/
retcode_t tb_Remove(Container_t C, ...) {
	tb_Key_t key;
	va_list parms;
	void *p;
	if(! tb_valid(C, TB_CONTAINER, __FUNCTION__))	return TB_ERR;
	
	va_start(parms, C);
	key = va_arg(parms, tb_Key_t);

	if((p = tb_getMethod(C, OM_REMOVE))) {
		return ((retcode_t(*)(tb_Object_t, tb_Key_t))p)(C, key); 
	}

	return TB_ERR;
}



Iterator_t tb_Find(Container_t C, tb_Key_t K) {
	retcode_t (*p)(Iterator_t, tb_Key_t);
	Iterator_t I = tb_Iterator(C);
	if(I) {
		if((p = tb_getMethod(C, OM_FIND))) {
			if(p(I, K) == TB_OK) {
				return I;
			}
		}
		tb_Free(I);
	}
	return NULL;
}

