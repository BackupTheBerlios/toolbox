//======================================================
// $Id: Iterators.c,v 1.1 2004/05/12 22:04:49 plg Exp $
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
 * @file Iterators.c Containers generic iterators
 */

/**
 * @defgroup Iterator Iterator_t
 * Iterators refers to a Container_t, and provide a convenient sequenced manipulator. 
 *Iterators members are current key and value of their Container_t target. Methods allows to step forward or backward. Containers are not aware of Iterators, so take care not to destroy, or even change a Container contents while grokking inside with an Iterator.
 * @ingroup Composite
 */

#include "tb_global.h"
#include "Toolbox.h"
#include "Objects.h"
#include "Vector.h"
#include "Containers.h"
#include "Memory.h"
#include "Error.h"
#include "Iterator.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"
#include "classRegistry.h"

inline iterator_members_t XIterator(Iterator_t I) {
	return (iterator_members_t)((__members_t)tb_getMembers(I, TB_ITERATOR))->instance;
}



void __build_iterator_once(int OID) {
	tb_registerMethod(OID, OM_FREE,               tb_iterator_free);
	tb_registerMethod(OID, OM_GETSIZE,             tb_iterator_getsize);
	//CODE-ME:	registerNewMethod(OID, OM_CLONE,              xxx);

	tb_implementsInterface(OID, "Iterable", 
												 &__iterable_build_once, build_iterable_once);
	tb_registerMethod(OID, OM_GET_ITERATOR_CTX,  _Iterator_getIterCtx);
}



void *tb_iterator_free(Iterator_t It) {
	iterator_members_t m = XIterator(It);
	void (*freeIter)(Iterator_t) = tb_getMethod(m->target, OM_FREE_ITERATOR_CTX);
	if(freeIter) 	freeIter(It);
	else  tb_warn("tb_iterator_free: can't find free IterCtx method\n"); 
	tb_freeMembers(It);
	return tb_getParentMethod(It, OM_FREE);
}



_iterator_ctx_t _Iterator_getIterCtx(Iterator_t It) {
	return (_iterator_ctx_t)XIterator(It)->Ctx;
}

int tb_iterator_getsize(Iterator_t It) {
	return XIterator(It)->size;
}



///////// Public interface

void *__getIterCtx(Iterator_t It) {
	return XIterator(It)->Ctx;
}


Iterator_t dbg_tb_iterator(char *func, char *file, int line, Container_t Cn) {
	set_tb_mdbg(func, file, line);
	return tb_Iterator(Cn);
}


/**
 * Iterator_t constructor.
 * 
 *
 * @param C : target Container_t
 * @return new Hash_t object
 * @see Container, Ktypes.h
 * \ingroup Iterator
 */
Iterator_t tb_Iterator(Container_t C) {
	Iterator_t It;
	_iterator_ctx_t (*newIterCtx)(Container_t);
	_iterator_ctx_t Ctx = NULL;
	iterator_members_t m;

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	no_error;

	if(! __VALID_IFACE(tb_isA(C), __interface_id_of("Iterable"))) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		tb_error("Object %s@%p doesn't implements interface 'Iterable'\n", 
						 tb_nameOf(tb_isA(C)), C);
		return NULL;
	}

	newIterCtx = tb_getMethod(C, OM_NEW_ITERATOR_CTX);
	if((Ctx = newIterCtx(C)) !=NULL) {
		It                     = tb_newParent(TB_ITERATOR);
		It->isA                = TB_ITERATOR;
		m = (iterator_members_t)tb_xcalloc(1, sizeof(struct iterator_members));  
		It->members->instance  = m;
		m->Ctx                 = Ctx;
		m->target              = C;
		m->size                = tb_getSize(C);

		if(fm->dbg) fm_addObject(It);

		return It;
	}
 
	return NULL; // never returns an empty iterator
}



/**
 * Rewind iterator to first position.
 * @param It : Iterator_t target
 * @return retcode
 * @see tb_Last, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
retcode_t tb_First(Iterator_t It) { 
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__) ) {
		retcode_t (*goFirst)(Iterator_t);

		goFirst = tb_getMethod(XIterator(It)->target, OM_GOFIRST);

		return goFirst(It);
	}
	return TB_ERR;
}


/**
 * Set iterator to last position.
 * @param It : Iterator_t target
 * @return retcode
 * @see tb_First, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
retcode_t tb_Last(Iterator_t It) { 
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__) ) {
		retcode_t (*goLast)(Iterator_t);

		goLast = tb_getMethod(XIterator(It)->target, OM_GOLAST);
	
		return goLast(It);
	}
	return TB_ERR;
}


/**
 * Set iterator to previous element.
 * @param It : Iterator_t target
 * @return the previous Object in referenced container
 * @see tb_Last, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
tb_Object_t tb_Prev(Iterator_t It) { 
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__) ) {
		tb_Object_t (*goPrev)(Iterator_t);

		goPrev = tb_getMethod(XIterator(It)->target, OM_GOPREV);

		return goPrev(It);
	}
	return NULL;
}


/**
 * Set iterator to next element.
 * @param It : Iterator_t target
 * @return the target object
 *
 * Example :
 * \code
 *	for(tb_First(D); ! tb_isLast(D); tb_Next(D)) {
 *		tb_profile("k[%s] ==> v[%S]\n", tb_Key(D), tb_Value(D));
 *	}
 * \endcode
 * @see tb_Last, tb_isLast, tb_isFirst, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
tb_Object_t tb_Next(Iterator_t It) { 
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__)) {
		tb_Object_t (*goNext)(Iterator_t);
	
		goNext = tb_getMethod(XIterator(It)->target, OM_GONEXT);

		return goNext(It);
	}
	return NULL;
}


/**
 * Accessor to the current Iterator pointed object's key.
 * @param It : Iterator_t target
 * @return the target object's key (Vector_t have no key, but an index)
 * @see tb_Last, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
tb_Key_t tb_Key(Iterator_t It) {
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__) ) {
		tb_Key_t (*curKey)(Iterator_t);

		curKey = tb_getMethod(XIterator(It)->target, OM_CURKEY);
	
		return curKey(It);
	}
	return KINVAL;
}


/**
 * Accessor to the current Iterator pointed object's value
 * @param It : Iterator_t target
 * @return the target object's value
 * @see tb_Last, tb_Next, tb_Prev, tb_Key, tb_Value
 * @ingroup Iterator
 */
tb_Object_t tb_Value(Iterator_t It) { 
	if(tb_valid(It, TB_ITERATOR, __FUNCTION__)) {
		tb_Object_t (*curVal)(Iterator_t);

		curVal = tb_getMethod(XIterator(It)->target, OM_CURVAL);

		return curVal(It);
	}
	return NULL;
}


/* CODE-ME:

int tb_KeyCMP(Iterator_t It, tb_Key_t) --> return kcmp(tb_Key(It), tb_Key_t tested)

*/
	

