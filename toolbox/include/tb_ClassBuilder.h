/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: tb_ClassBuilder.h,v 1.1 2004/05/12 22:04:49 plg Exp $
//======================================================

// created on Thu Aug  1 15:23:25 2002

/* Copyright (c) 1999-2004, Paul L. Gatille. All rights reserved.
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

#ifndef __TB_CLASSBUILDER_H
#define __TB_CLASSBUILDER_H

#include "Toolbox.h"

retcode_t __VALID_CLASS                     (int Oid, int Cid);
retcode_t __VALID_IFACE                     (int Oid, int Iid);


int       tb_registerNewClass               (char *name, int parent, void (*init)(int Cid));
int       tb_registerNewInterface           (char *name);
int       tb_registerNew_InterfaceMethod    (char *name, int Iid);
int       tb_registerNew_ClassMethod        (char *name, int Oid);
void      tb_registerMethod                 (int Cid, int Mid, void *fnc);
void      tb_implementsInterface            (int Cid, char * interface_name, 
																						 pthread_once_t *interface_po, 
																						 void (*interface_builder)(void));
void      tb_classRegisterInit              (void);
void    * tb_getParentMethod                (tb_Object_t O, int Mid);

void    * __getMethod                       (int Cid, int Mid);
void    * __getParentMethod                 (int Cid, int Mid);

extern pthread_once_t __class_registry_init_once;

#endif
