//======================================================
// $Id: Dict.c,v 1.2 2004/05/24 16:37:52 plg Exp $
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
 * @file Dict.c Dict_t methods
 */

/**
 * @defgroup Dict Dict_t
 * @ingroup Container
 * Dict_t objects are sorted collections of key/values pairs.
 *
 * Current implementation is based on B+ trees, a self-balancing tree with a large branch factor. Characteristics are good to very good insert/access performance, consistent sorted linking of leafs (allows fast sorted access to previous or subsequent keys of a arbitrary search), and average remove performance (to be optimized). 
 * \todo fix the 'dupes' mess
 * \todo code file store/access
 * \todo code clear method
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <pthread.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "Dict.h"
#include "bplus_tree.h"
#include "Memory.h"
#include "Error.h"
#include "tb_ClassBuilder.h"




Dict_t dbg_tb_dict(char *func, char *file, int line, int type, int dupes_allowed) {
	set_tb_mdbg(func, file, line);
	return tb_dict_new(type, dupes_allowed);
}


/**
 * Dict_t constructor.
 * \ingroup Dict
 * @param type : key type (string/int)
 * @param dupes_allowed : boolean value, allows or not duplicates keys
 * @return new Dict_t object
 * \sa Container, Ktypes.h
 */
Dict_t tb_Dict(int type, int dupes_allowed) {
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	return tb_dict_new(type, dupes_allowed);
}



