//------------------------------------------------------------------
// $Id: Pointer.c,v 1.1 2004/05/12 22:04:52 plg Exp $
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

#include "Memory.h"

inline pointer_members_t XPtr(Pointer_t P) {
	return (pointer_members_t)((__members_t)tb_getMembers(P, TB_POINTER))->instance;
}

//#define XPTR(P) ((void *)P->members->instance)

void __build_pointer_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_Pointer);
	tb_registerMethod(OID, OM_FREE,         tb_pointer_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_pointer_getsize);
	tb_registerMethod(OID, OM_DUMP,         tb_pointer_dump);
}


void *P2p(Pointer_t P) {
	if(tb_valid(P, TB_POINTER, __FUNCTION__)) {
		return XPtr(P)->userData;
	}
	return NULL;
}

Pointer_t dbg_tb_pointer(char *func, char *file, int line, void *v, void *free_fnc) {
	set_tb_mdbg(func, file, line);
	return tb_Pointer(v, free_fnc);
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
	tb_Object_t This;
	pointer_members_t m;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	This = tb_newParent(TB_POINTER); 
	This->isA = TB_POINTER;
	This->members->instance = (pointer_members_t)tb_xcalloc(1, sizeof(struct pointer_members));
	m = (pointer_members_t)This->members->instance;
	m->size = -1;
	if(free_fnc) m->freeUserData = (void (*)(void*))free_fnc;
	m->userData = v;
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




