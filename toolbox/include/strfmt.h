// $Id: strfmt.h,v 1.1 2004/05/12 22:04:49 plg Exp $
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

#ifndef STRFMT_H
#define STRFMT_H
int __tb_asprintf(char **s, const char *fmt, ...);
int __tb_vasprintf(char **s, const char *fmt, va_list ap);
int __tb_nasprintf(char **s, unsigned int max, const char *fmt, ...);
int __tb_vnasprintf(char **s, unsigned int max, const char *fmt, va_list ap);

#endif
