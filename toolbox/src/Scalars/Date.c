/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//------------------------------------------------------------------
// $Id: Date.c,v 1.1 2004/05/12 22:04:52 plg Exp $
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

/**
 * @file Date.c
 */

/**
 * @defgroup DATE Date_t
 * @ingroup TB_SCALAR
 * Class Date (extends TB_SCALAR, [implements interface ...])
 * description :
 *
 * ... here the header description
 */

/* uncomment only for internal Toolbox Class 
#ifndef __BUILD
#define __BUILD
#endif
*/

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Date.h"
#include "Date_impl.h"
#include "tb_ClassBuilder.h"


/* 

	specific public class and related methods goes here

*/
retcode_t Dt_setabsolute(Date_t T, time_t val) {
}
retcode_t Dt_setbroken_down(Date_t T, struct tm val) {
}


time_t Dt_getabsolute(Date_t T) {
}
struct tm *Dt_getbroken_down(Date_t T) {
}


time_t iso8601_to_time(char *iso8601) {
	//	"19930214T131030" or "1993-02-14T13:10:30" 
	Vector_t V = tb_Vector();
	String_t S = tb_String("%s", iso8601);
	time_t  time = 0;
	struct tm stm;
	if(tb_getRegex(V, S, 
							"(\\d{4})[-]?(\\d{2})[-]?(\\d{2})T(\\d{2})[:]?(\\d{2})[:]?(\\d{2})",
								 PCRE_MULTI) == 6) {

		if((stm.tm_year = tb_toInt(V, 0)) <= 0) goto skip;
		stm.tm_mon = tb_toInt(V, 1);
		if(!(stm.tm_mon >=0 && stm.tm_mon <=11)) goto skip;
		stm.tm_mday = tb_toInt(V, 2);
		if(!(stm.tm_mon >=1 && stm.tm_mon <=31)) goto skip;

		stm.tm_hour = tb_toInt(V, 3);
		if(!(stm.tm_hour >=0 && stm.tm_hour <=23)) goto skip;
		stm.tm_min = tb_toInt(V, 4);
		if(!(stm.tm_min >=0 && stm.tm_hour <=59)) goto skip;
		stm.tm_sec = tb_toInt(V, 4);
		if(!(stm.tm_sec >=0 && stm.tm_sec <=59)) goto skip;

		time = mktime(&stm);
	}
 skip:
	tb_Free(V);
	tb_Free(S);

	return time;
}
