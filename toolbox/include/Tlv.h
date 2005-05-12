//------------------------------------------------------------------
// 	$Id: Tlv.h,v 1.1 2005/05/12 21:54:47 plg Exp $
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

#ifndef __TB_TLV_H
#define __TB_TLV_H

typedef void *Tlv_t;

Tlv_t Tlv          (int type, int len, char *value);
void  Tlv_free     (Tlv_t T);
inline int   Tlv_getType  (Tlv_t T);
inline int   Tlv_getLen   (Tlv_t T);
inline int   Tlv_getFullLen(Tlv_t T);
inline char *Tlv_getValue (Tlv_t T);

#endif
