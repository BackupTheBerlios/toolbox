//=================================================================
// $Id: Composites.c,v 1.1 2004/05/12 22:04:49 plg Exp $
//=================================================================
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
 * @file Composites.c Composite_t definitions
 */

/**
 * @defgroup Composite Composite types
 * Complex aggregates of structs and scalars
 * @ingroup Object
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "tb_global.h"
#include "Objects.h"
#include "Composites.h"
#include "tb_ClassBuilder.h"

void __build_composite_once(int OID) {
	tb_registerMethod(OID, OM_NEW,  tb_composite_new);
	tb_registerMethod(OID, OM_FREE, tb_composite_free);
	tb_registerMethod(OID, OM_DUMP, tb_composite_dump);
}



Composite_t tb_composite_new() {
	tb_Object_t O;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	O = tb_newParent(TB_COMPOSITE);
	O->isA = TB_COMPOSITE;
	return O;
}

void *tb_composite_free(Composite_t C) {
	if(tb_valid(C, TB_COMPOSITE, __FUNCTION__)) {
		C->isA = TB_COMPOSITE;
		tb_freeMembers(C);
		return tb_getParentMethod(C, OM_FREE);
	}
	return NULL;
}


void tb_composite_dump( Composite_t O, int level ) {
	int i;
	if(! tb_valid(O, TB_COMPOSITE, __FUNCTION__)) return;
	
	for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, "<%s TYPE=\"%d\" REFCNT=\"%d\" DOCKED=\"%d\" ADDR=\"%p\"/>\n",
					tb_nameOf(O->isA), O->isA, O->refcnt, O->docked, O);
}





