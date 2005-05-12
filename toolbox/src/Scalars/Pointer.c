//------------------------------------------------------------------
// $Id: Pointer.c,v 1.4 2005/05/12 21:52:12 plg Exp $
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

/**
 * @file Pointer.c 
 */

/**
 * @defgroup Pointer Pointer_t
 * @ingroup Scalar
 * Pointer (user data pointer) related methods and functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "Pointer.h"
#include "Objects.h"
#include "tb_ClassBuilder.h"
#include "Serialisable_interface.h"
#include "Tlv.h"

#include "Memory.h"

inline pointer_members_t XPtr(Pointer_t P) {
	return (pointer_members_t)((__members_t)tb_getMembers(P, TB_POINTER))->instance;
}

//#define XPTR(P) ((void *)P->members->instance)


static Tlv_t     tb_pointer_toTlv    (Pointer_t Self);
static Pointer_t tb_pointer_fromTlv  (Tlv_t T);

void __build_pointer_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          Pointer_new);
	tb_registerMethod(OID, OM_FREE,         tb_pointer_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_pointer_getsize);
	tb_registerMethod(OID, OM_DUMP,         tb_pointer_dump);
	tb_registerMethod(OID, OM_CLEAR,        Pointer_clear);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);
	tb_registerMethod(OID, OM_TOTLV,        tb_pointer_toTlv);
	tb_registerMethod(OID, OM_FROMTLV,      tb_pointer_fromTlv);

}


void *P2p(Pointer_t P) {
	if(tb_valid(P, TB_POINTER, __FUNCTION__)) {
		return XPtr(P)->userData;
	}
	return NULL;
}

Pointer_t dbg_tb_pointer(char *func, char *file, int line, void *v, void *free_fnc) {
	set_tb_mdbg(func, file, line);
	return Pointer_ctor(Pointer_new(), v, free_fnc);
}

/** Pointer_t constructor.
 * Allocate and initialise a container for a void * C pointer
 * 
 * Pointer_t is basically an encapsulated void *, allowing to store user defined structures into Toolbox's types without the hassle of defining a full class (see tb_registerNewClass).
 * Optional destroy parameter is called on data upon Pointer_t destruction.
 * If destroy is not NULL, it must be a function of type void (*f)(void *)
 *
 * Example :
 * \code
 * struct mystruct {
 *  char *titi;
 *	int toto;
 * };
 *
 * void myfree(void *arg) {
 *  struct mystruc *p = (struct mystruc *)arg;
 * 	if(p->titi) free(p->titi);
 * 	free(arg);
 * }
 *
 * ...
 *
 * int main() {
 * 
 * struct mystruc *p = calloc(1, sizeof(struct mystruc));
 * Pointer_t P = tb_Pointer(p, myfree);
 *
 * \endcode
 * @param v : void * pointer to user data
 * @param free_fnc : destructor method, or NULL
 * @return newly allocated Pointer_t
 * @see Object, Scalar, 
 * @ingroup Pointer
 */
Pointer_t tb_Pointer(void *v, void *free_fnc) {
	return Pointer_ctor(Pointer_new(), v, free_fnc);
}

Pointer_t Pointer_ctor(Pointer_t Self, void *p, void *free_fnc) {
	if(tb_valid(Self, TB_POINTER, __FUNCTION__)) {
		pointer_members_t m = XPtr(Self);
		if(m->userData && m->freeUserData) {
#ifdef TB_MEM_DEBUG
			if(m->freeUserData == tb_xfree ) {
				tb_xfree(m->userData);
			} else {
				m->freeUserData(m->userData);
			}
#else
			m->freeUserData(m->userData);
#endif				
		}

		m->userData = p;
		m->freeUserData =  (void (*)(void*))free_fnc;
	}
	return Self;
}

Pointer_t Pointer_new() {
	tb_Object_t This;
	pointer_members_t m;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	This = tb_newParent(TB_POINTER); 
	This->isA = TB_POINTER;
	This->members->instance = (pointer_members_t)tb_xcalloc(1, sizeof(struct pointer_members));
	m = (pointer_members_t)This->members->instance;
	m->size = -1;

	if(fm->dbg) fm_addObject(This);

	return This;
}


int tb_pointer_getsize(Pointer_t P) {
	return XPtr(P)->size;
}

void *tb_pointer_free(Pointer_t P) {
	if(tb_valid(P, TB_POINTER, __FUNCTION__)) {
		pointer_members_t m = XPtr(P);
		if(fm->dbg) {
			if(m->freeUserData && m->freeUserData == tb_xfree) {
				tb_xfree(m->userData);
			} else {
#ifdef TB_MEM_DEBUG
				// else freeing won't be recorded, causing false leak
				if(m->freeUserData){
					if(m->freeUserData == tb_xfree ) {
						tb_xfree(m->userData);
					} else {
						m->freeUserData(m->userData);
					}
				}
#else
				if(m->freeUserData) m->freeUserData(m->userData);
#endif				
			}
		} else {
			if(m->freeUserData) m->freeUserData(m->userData);
		}
		m->userData = NULL; // anyway, wipe out ptr ref
		tb_freeMembers(P);
		P->isA = TB_POINTER;
		return tb_getParentMethod(P, OM_FREE);
	}
	return NULL;
}

void tb_pointer_dump(Pointer_t P, int level) {
	int i;
	if(tb_valid(P, TB_POINTER, __FUNCTION__)) {
		pointer_members_t m = XPtr(P);
		for(i = 0; i<level; i++) fprintf(stderr, " ");
		fprintf(stderr, 
						"<TB_POINTER SIZE=\"%d\" ADDR=\"%p\" DATA=\"%p\" FREE_FNC=\"%p\" REFCNT=\"%d\" />\n",
						m->size, P, m->userData, m->freeUserData, P->refcnt);
	}
}


Pointer_t Pointer_clear(Pointer_t Self) {
	if(tb_valid(Self, TB_POINTER, __FUNCTION__)) {
		pointer_members_t m = XPtr(Self);
		if(m->userData && m->freeUserData) {
#ifdef TB_MEM_DEBUG
			if(m->freeUserData == tb_xfree ) {
				tb_xfree(m->userData);
			} else {
				m->freeUserData(m->userData);
			}
#else
			m->freeUserData(m->userData);
#endif				
			m->userData = NULL;
			m->freeUserData = NULL;
		}
	}
	return Self;
}



Tlv_t tb_pointer_toTlv(Pointer_t Self) {
	char dummy = 0x00;
	return Tlv(TB_POINTER, 1, &dummy);
}

Pointer_t tb_pointer_fromTlv(Tlv_t T) {
	return tb_Pointer(NULL,NULL);
}
