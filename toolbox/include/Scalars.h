//------------------------------------------------------------------
// $Id: Scalars.h,v 1.2 2004/07/01 21:37:18 plg Exp $
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

#ifndef __TB_SCALARS_H
#define __TB_SCALARS_H

#include "Toolbox.h"
#include "Objects.h"


// clean-me: all this should be static
tb_Object_t  tb_scalar_new   (void);
void       * tb_scalar_free  (Scalar_t S);
void         tb_scalar_dump  (tb_Object_t O, int level);

void         tb_string_marshall     (String_t marshalled, String_t S, int level);
String_t     tb_string_unmarshall   (XmlElt_t xml_entity);

void         tb_num_marshall        (String_t marshalled, Num_t N, int level);
Num_t        tb_num_unmarshall      (XmlElt_t xml_entity);

void         tb_raw_marshall        (String_t marshalled, Raw_t R, int level);
Raw_t        tb_raw_unmarshall      (XmlElt_t xml_entity);

#endif



