/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Date.h,v 1.1 2004/05/12 22:04:52 plg Exp $
//======================================================

// created on Sun May  9 15:48:00 2004

/* Copyright (c) 2004, Paul Gatille <paul.gatille@free.fr>
 *
Copyright (c) 2004, Audemat-Aztec http://audemat-aztec.com
 *
 */

// Class Date public methods

#ifndef __DATE_H
#define __DATE_H

#include "Toolbox.h"
#include "Scalars.h"

typedef tb_Object_t           Date_t;

extern int TB_DATE;

// --[public methods goes here ]--

// constructors
Date_t    Date(char *iso8601);
Date_t    Date_new(char *iso8601);
// factories (produce new object(s))
/*...*/
// manipulators (change self) 
retcode_t Dt_setabsolute(Date_t T, time_t val);
retcode_t Dt_setbroken_down(Date_t T, struct tm val);

/*...*/
// inspectors (don't change self) 
time_t    Dt_getabsolute(Date_t T);
struct tm Dt_getbroken_down(Date_t T);

int       tb_DateCmp        (Date_t Dt1, Date_t Dt2);
time_t    tb_DateElapsed    (Date_t Dt1, Date_t Dt2);
retcode   tb_DateAddDays    (Date_t Dt, int days);
retcode   tb_DateAddSecs    (Date_t Dt, int seconds);
int       tb_secs2days      (int seconds);

/*...*/

#endif

