//====================================================
// $Id: Scalars.c,v 1.1 2004/05/12 22:04:52 plg Exp $
//====================================================
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


/** @file Scalars.c
 */

/**
 * @defgroup Scalar Scalar types
 * Embedding and extension of basic types
 * @ingroup Object
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "tb_global.h"
#include "Scalars.h"
#include "C_Castable_interface.h"
#include "tb_ClassBuilder.h"


void __build_scalar_once(int OID) {
	tb_registerMethod(OID, OM_NEW,  tb_scalar_new);
	tb_registerMethod(OID, OM_FREE, tb_scalar_free);
	tb_registerMethod(OID, OM_DUMP, tb_scalar_dump);
}

tb_Object_t tb_scalar_new() {
	tb_Object_t O;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	O = tb_newParent(TB_SCALAR);
	O->isA = TB_SCALAR;
	return O;
}

void *tb_scalar_free(Scalar_t S) {
	if(TB_VALID(S, TB_SCALAR)) {
		S->isA = TB_SCALAR;
		tb_freeMembers(S);
		return tb_getParentMethod(S, OM_FREE);
	}
	return NULL;
}

void tb_scalar_dump( tb_Object_t O, int level ) {
	if(! TB_VALID(O, TB_SCALAR)) {
		tb_warn("Dump: %x not a tb_Scalar\n");
		return;
	}
	fprintf(stderr, "<TB_SCALAR TYPE=\"%d\" ADDR=\"%p\"/>\n",
					O->isA, O);
}






