/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: XmlRpc_impl.h,v 1.1 2004/05/12 22:04:53 plg Exp $
//======================================================

// created on Tue May 11 23:37:45 2004 by Paul Gatille <paul.gatille\@free.fr>

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

// Class XmlRpc private and internal methods and members


#ifndef __XMLRPC_IMPL_H
#define __XMLRPC_IMPL_H

#include "Toolbox.h"
#include "XmlRpc.h"


struct XmlRpc_members {
  Hash_t signatures;
};
typedef struct XmlRpc_members * XmlRpc_members_t;



inline XmlRpc_members_t XXmlRpc(XmlRpc_t T);

#if defined TB_MEM_DEBUG && (! defined NDEBUG) && (! defined __BUILD)
XmlRpc_t dbg_XmlRpc(char *func, char *file, int line);
#define XmlRpc(x...)      dbg_XmlRpc(__FUNCTION__,__FILE__,__LINE__,x)
#endif

#endif

