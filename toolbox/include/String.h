//------------------------------------------------------------------
// $Id: String.h,v 1.1 2004/05/12 22:04:48 plg Exp $
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

#ifndef __TB_STRING_H
#define __TB_STRING_H

#include "Toolbox.h"
#include "Objects.h"
struct   string_members {
	char   * data;
	int      size;
	int      allocated;
	int      grow_factor;
	//RFU-NYI	int      charset;
	//RFU-NYI	int      encoding;
};
typedef struct string_members *string_members_t;
inline string_members_t XStr(String_t This);
//#define _SZ(A)      ((char *)((String_t)A)->data)


void     tb_string_marshall   (String_t marshalled, String_t S, int level);
String_t tb_string_unmarshall (XmlElt_t xml_entity);
#endif
