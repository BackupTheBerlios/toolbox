//------------------------------------------------------------------
// $Id: Objects.h,v 1.5 2005/05/12 21:54:36 plg Exp $
//------------------------------------------------------------------
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

//

#ifndef __TB_OBJECTS_H
#define __TB_OBJECTS_H

/** 
 * @file Objects.h Objects base definitions
 */
#include <pthread.h>
extern pthread_once_t __class_registry_init_once;


struct __members {
	int                 instance_type;
	void              * instance;
	struct __members  * parent;
};
//typedef struct __members *__members_t; //intentionnaly removed (include tb_ClassBuilder.h instead)

/**
 * tb_Object internals.
 * \ingroup Object
 * \warning Informational only. Don't mess with this directly.
 * @see tb_Object_t
 */
struct tb_Object {
  unsigned int        isA;     /**< object type */
  unsigned int       refcnt;  /**< reference counter for aliases */
	int                docked;  /**< docking (inserted in container) counter */

	//  size_t              size;    /**< returned by tb_getSize (nb elmts, size of string ...) */
	//  void              * data;    /**< any kind of data required */
	struct __members  * members; /**< members storage */
};
//typedef struct tb_Object* tb_Object_t; // intentionnaly removed (include Toolbox.h instead)


// fixme: not a macro : either inline, or move to virtual methods
#define TB_DOCK(A)         (((tb_Object_t)(A))->docked ++)
#define TB_UNDOCK(A)       (((tb_Object_t)(A))->docked --)


extern int  OM_NEW;
extern int  OM_FREE;
extern int	OM_GETSIZE;
extern int	OM_CLONE;
extern int	OM_DUMP;
extern int	OM_CLEAR;
extern int	OM_COMPARE;
extern int	OM_SET;
extern int	OM_INSPECT;
extern int	OM_STRINGIFY;
extern int	OM_SERIALIZE;
extern int	OM_UNSERIALIZE;

extern int  OM_NUMSET;

// fixme: not a macro!
#define PTBOBJ(A)    ((tb_Object_t *)(A)) 


int                tb_valid           (void *Object, int tb_type, char *function_name);
struct tb_Object * tb_newParent       (int child);
void             * tb_getMethod       (struct tb_Object *O, int OM_NDX);
const char       * tb_nameOf          (int isA);
const char       * tb_methodName      (int met);
inline int         tb_getClassIdByName(char *class_name);
#endif













