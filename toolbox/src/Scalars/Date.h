/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Date.h,v 1.2 2004/05/13 22:07:05 plg Exp $
//======================================================

// created on Sun May  9 15:48:00 2004

/* Copyright (c) 2004, Paul Gatille <paul.gatille@free.fr>
 *
 *
 */

// Class Date public methods

#ifndef __DATE_H
#define __DATE_H

#include "Toolbox.h"
#include "Scalars.h"

// --[public methods goes here ]--

// constructors
Date_t       tb_Date           (char *iso8601);
Date_t       Date_new          (char *iso8601);
// manipulators (change self) 
retcode_t    Dt_setAbsolute    (Date_t T, time_t val);
retcode_t    Dt_setBrokenDown  (Date_t T, struct tm *val);

// inspectors (don't change self) 
time_t       Dt_getAbsolute    (Date_t T);
struct tm  * Dt_getBrokenDown  (Date_t T, struct tm *val);

int          tb_DateCmp        (Date_t Dt1, Date_t Dt2);

time_t       iso8601_to_time   (char *iso8601);


Date_t       Dt_getYear        (Date_t This);
Date_t       Dt_getMonth       (Date_t This);
Date_t       Dt_getMday        (Date_t This);
Date_t       Dt_getWday        (Date_t This);
Date_t       Dt_getWeek        (Date_t This);
Date_t       Dt_getDOY         (Date_t This);

retcode_t    Dt_calcDelta      (Date_t Dt1, Date_t Dt2, 
																&nb_years, 
																&nb_monthes, 
																&nb_days, 
																&nb_hours, 
																&nb_mins, 
																&nb_secs);
Date_t       Dt_addYears       (Date_t This, int nb);
Date_t       Dt_subYears       (Date_t This, int nb);
Date_t       Dt_addMonthes     (Date_t This, int nb);
Date_t       Dt_subMonthes     (Date_t This, int nb);
Date_t       Dt_addDays        (Date_t This, int nb);
Date_t       Dt_subDays        (Date_t This, int nb);
Date_t       Dt_addHours       (Date_t This, int nb);
Date_t       Dt_subHours       (Date_t This, int nb);
Date_t       Dt_addMins        (Date_t This, int nb);
Date_t       Dt_subMins        (Date_t This, int nb);
Date_t       Dt_addSecs        (Date_t This, int nb);
Date_t       Dt_subSecs        (Date_t This, int nb);


#endif


