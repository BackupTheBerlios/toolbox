////////////////////////////////////////////////////////////////////////////
// $Id: version.c,v 1.1 2004/05/12 22:04:49 plg Exp $
////////////////////////////////////////////////////////////////////////////


/* Copyright (c) 1999,2000,2001, Paul L. Gatille. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the "Artistic License" which comes with this Kit. 
 *
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 *
 *
 * You should have received a copy of the Artistic License with this Kit, 
 * in the file named "Artistic License". If not, I'll be glad to provide one.
 */



#include "version.h"
#include "build.h"

#ifndef TB_VERSION
#define TB_VERSION "who cares ?"
#endif
char *tb_getVersion(void) {
        return TB_VERSION;
}      
char *tb_getBuild(void) {
        return "nyi";
}  
