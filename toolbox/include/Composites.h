//------------------------------------------------------------------
// $Id: Composites.h,v 1.1 2004/05/12 22:04:47 plg Exp $
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

#ifndef __TB_COMPOSITE_H
#define __TB_COMPOSITE_H

#include "Toolbox.h"
#include "Objects.h"

Composite_t   tb_composite_new  (void);
void        * tb_composite_free (Composite_t C);
void          tb_composite_dump (Composite_t C, int level );

#endif
