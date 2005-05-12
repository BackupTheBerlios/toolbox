//------------------------------------------------------------------
// $Id: String.h,v 1.3 2005/05/12 21:54:36 plg Exp $
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


int __tb_asprintf     (char **s, const char *fmt, ...);
int __tb_vasprintf    (char **s, const char *fmt, va_list ap);
int __tb_vnasprintf   (char **s, unsigned int max, const char *fmt, va_list ap);


void     tb_string_marshall   (String_t marshalled, String_t S, int level);
String_t tb_string_unmarshall (XmlElt_t xml_entity);

void     stringFactoryCleanCache();
#endif
