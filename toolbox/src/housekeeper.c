//------------------------------------------------------------------
// $Id: housekeeper.c,v 1.1 2005/05/12 21:48:22 plg Exp $
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

#include "Toolbox.h"
#include "tb_ClassBuilder.h"
#include "String.h"

int __full_debug__ = 0;

__attribute__ ((constructor)) void tbx_init()
{
	/* code here is executed after dlopen() has loaded the module */
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	char *s = getenv("tb_full_debug");
	if(s && atol(s) == 1) {
		__full_debug__ = 1;
	}

	tb_info("in tbx ctor\n");
}


__attribute__ ((destructor)) void tbx_fini()
{
	/* code here is executed just before dlclose() unloads the module */
	tb_info("in tbx dtor\n");
	stringFactoryCleanCache();

}
