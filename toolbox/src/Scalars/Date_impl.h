/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Date_impl.h,v 1.1 2004/05/12 22:04:53 plg Exp $
//======================================================

// created on Sun May  9 15:48:00 2004 by Paul Gatille <paul.gatille@free.fr>

/* 
 * Copyright (c) 2004, Audemat-Aztec http://audemat-aztec.com
 */

// Class Date private and internal methods and members


#ifndef __DATE_IMPL_H
#define __DATE_IMPL_H

#include "Toolbox.h"
#include "Date.h"


struct Date_members {
  time_t absolute;
  struct tm broken_down;
};
typedef struct Date_members * Date_members_t;



inline Date_members_t XDate(Date_t T);

#if defined TB_MEM_DEBUG && (! defined NDEBUG) && (! defined __BUILD)
Date_t dbg_Date(char *func, char *file, int line, char *iso8601);
#define Date(x...)      dbg_Date(__FUNCTION__,__FILE__,__LINE__,x)
#endif

#endif

