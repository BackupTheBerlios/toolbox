// $Id: Num.h,v 1.2 2004/07/01 21:37:09 plg Exp $
//================================================================
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

#ifndef _NUM_H
#define _NUM_H

#include "Toolbox.h"


struct num_members {
	char strbuff[20];
	char sign; // 0 = signed
	char NaN;  // 0 = valid num
	int value;
	int size;
};
typedef struct num_members *num_members_t;


#endif




