/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//------------------------------------------------------------------
// $Id: Date.c,v 1.3 2004/05/24 16:37:52 plg Exp $
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
 * Class Date (extends TB_SCALAR, implements C-Castable & Serializable)
 * description : 
 *
 * ... here the header description
 */

#ifndef __BUILD
#define __BUILD
#endif

#include "Date_impl.h"
#include "Memory.h"
#include "Error.h"
#include "tb_ClassBuilder.h"


/* 

	specific public class and related methods goes here

*/


/** Assign Date in seconds from epoch (january, 1st 1970)
 *
 */
retcode_t Dt_setAbsolute(Date_t T, time_t val) {
	if(tb_valid(T, TB_DATE, __FUNCTION__) && val >0) {
		Date_members_t m = XDate(T);
		m->absolute = val;
		localtime_r(&(m->absolute), &(m->broken_down));
		snprintf(m->string, 20, "%d%02d%02dT%02d:%02d:%02d",
						 m->broken_down.tm_year + 1900,
						 m->broken_down.tm_mon,
						 m->broken_down.tm_mday,
						 m->broken_down.tm_hour,
						 m->broken_down.tm_min,
						 m->broken_down.tm_sec);

		return TB_OK;
	}
	return TB_KO;
}


retcode_t Dt_setBrokenDown(Date_t T, struct tm *val) {
	if(tb_valid(T, TB_DATE, __FUNCTION__) && val >0) {
		Date_members_t m = XDate(T);

		m->broken_down.tm_year  = val->tm_year;
		m->broken_down.tm_mon   = val->tm_mon;
		m->broken_down.tm_mday  = val->tm_mday;
		m->broken_down.tm_hour  = val->tm_hour;
		m->broken_down.tm_min   = val->tm_min;
		m->broken_down.tm_sec   = val->tm_sec;
		m->absolute = mktime(&(m->broken_down));
		snprintf(m->string, 20, "%d%02d%02dT%02d:%02d:%02d",
						 m->broken_down.tm_year + 1900,
						 m->broken_down.tm_mon,
						 m->broken_down.tm_mday,
						 m->broken_down.tm_hour,
						 m->broken_down.tm_min,
						 m->broken_down.tm_sec);

		return TB_OK;
	}
	return TB_KO;
}


time_t Dt_getAbsolute(Date_t T) {
	if(tb_valid(T, TB_DATE, __FUNCTION__)) return XDate(T)->absolute;
	
	return -1;
}

struct tm *Dt_getBrokenDown(Date_t T, struct tm *stm) {
	if(tb_valid(T, TB_DATE, __FUNCTION__)) {
		Date_members_t m = XDate(T);

		stm->tm_year  = m->broken_down.tm_year;
		stm->tm_mon   = m->broken_down.tm_mon;
		stm->tm_mday  = m->broken_down.tm_mday;
		stm->tm_hour  = m->broken_down.tm_hour;
		stm->tm_min   = m->broken_down.tm_min;
		stm->tm_sec   = m->broken_down.tm_sec;

		return stm;
	}
	return NULL;
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
		if((stm.tm_year = tb_toInt(V, 0) -1900) <= 0) goto skip;
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

	tb_warn("time=%d\n", time);
	return time;
}


int tb_DateCmp(Date_t Dt1, Date_t Dt2) {
	if(tb_valid(Dt1, TB_DATE, __FUNCTION__) &&
		 tb_valid(Dt2, TB_DATE, __FUNCTION__)) {
		return XDate(Dt1)->absolute - XDate(Dt2)->absolute;
	}
	return TB_ERR;
}

Date_t DateNow() {
	return Date_fromTime(time(NULL));
}
