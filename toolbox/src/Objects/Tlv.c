//------------------------------------------------------------------
// $Id: Tlv.c,v 1.1 2005/05/12 21:51:51 plg Exp $
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


#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "Toolbox.h"
#include "Memory.h"
#include "tb_global.h"
#include "String.h"
#include "tb_ClassBuilder.h"
#include "C_Castable_interface.h"
#include "Serialisable_interface.h"

#include "Memory.h"
#include "Error.h"



/* struct Tlv_rope { */
/* 	char  etx; */
/* 	int   major; */
/* 	int   minor; */
/* 	int   Len; */
/* 	char *Value; */
/* 	int   cksum; */
/* 	char  stx; */
/* } */
/* typedef struct Tlv_rope_struct *Tlv_rope_t; */


void *Tlv(int type, int len, char *value) {
	//	tb_warn("type=%d, len=%d, value=%p", type, len, value);
	if(value != NULL) {
		Tlv_t TLV  = tb_xcalloc(1, (sizeof(int)*2) + len);
		void *T = TLV;
		*(((int *)T)++) = type;
		*(((int *)T)++) = len;
		memcpy((char *)T, value, len);
		//		tb_hexdump(TLV, len+sizeof(int)*2);
		return TLV;
	}
	return NULL;
}

inline int Tlv_getType(Tlv_t T) {
	return *(int*)T;
}

inline int Tlv_getLen(Tlv_t T) {
	return *(((int*)T)+1);
}

inline int Tlv_getFullLen(Tlv_t T) {
	return (*(((int*)T)+1) + sizeof(int)*2);
}


inline char *Tlv_getValue(Tlv_t T) {
	return (char *)(((int*)T)+2);
}
