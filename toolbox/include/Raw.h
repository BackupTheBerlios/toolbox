// $Id: Raw.h,v 1.2 2004/07/01 21:37:18 plg Exp $
//======================================================
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

#ifndef __TB_RAW_H
#define __TB_RAW_H

#include "Toolbox.h"
#include "Objects.h"

struct raw_members {
	void *data;
	int   size;
	//	int   encoding;
};
typedef struct raw_members *raw_members_t;

inline raw_members_t XRaw(Raw_t R);
#endif
