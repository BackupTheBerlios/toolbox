//========================================================================
// 	$Id: Containers.h,v 1.2 2004/07/01 21:37:01 plg Exp $
//========================================================================
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

#ifndef __TB_CONTAINERS_H
#define __TB_CONTAINERS_H

#include <sys/types.h>

#include "Toolbox.h"
#include "Objects.h"


extern int OM_REPLACE;
extern int OM_INSERT;
extern int OM_GET;
extern int OM_EXISTS;
extern int OM_TAKE;
extern int OM_REMOVE;


/// default compare fnc for sorting objects.
int tb_default_cmp(tb_Object_t A, tb_Object_t B);

Container_t   tb_container_new  (void);
void        * tb_container_free (Container_t C);
void          tb_container_dump (Container_t C, int level );

#endif
