// $Id: Error.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//==========================================================
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

#ifndef TB_ERROR_H
#define TB_ERROR_H

void set_tb_errno(int e);
#define no_error (set_tb_errno(0))

#endif
