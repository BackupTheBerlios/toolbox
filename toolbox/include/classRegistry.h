/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: classRegistry.h,v 1.4 2005/05/12 21:54:36 plg Exp $
//======================================================

// created on Thu Aug  1 15:23:25 2002
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

#ifndef __CLASSREGISTRY_H
#define __CLASSREGISTRY_H

struct vtable {
	int       nb;
	void    * methods;
};
typedef struct vtable *vtable_t;

struct iface {
	int      Iid;
	vtable_t vtable;
};
typedef struct iface *iface_t;

struct ifaces {
	int        nb;
	iface_t  * interfaces;
};
typedef struct ifaces *ifaces_t;

struct classReg {
	int             Cid;
	int             parent_Cid;
	char          * name;
	vtable_t        class_methods;
	ifaces_t        ifaces_methods;
};
typedef struct classReg *classReg_t;

struct classRegister {
	int           classes_nb;
	classReg_t  * Register;
};
typedef struct classRegister *classRegister_t;

struct ifaceReg {
	int      Iid;
	int      methods_nb;
	char   * name;
};
typedef struct ifaceReg *ifaceReg_t;

struct ifacesRegister {
	int            interfaces_nb;
	ifaceReg_t   * Register;
};
typedef struct ifacesRegister *ifacesRegister_t;


enum typeofmeth {
	MT_UNDEF = 0,
	MT_CLASS,
	MT_INTERFACE
};




// qualified method is :  ownerId::name
struct methodReg {
	char             * name;
	enum typeofmeth    method_type;  // MT_CLASS | MT_INTERFACE
	int                ownerId;      // class id | interface id
	int                Mid;          // public, global and extern identifier
	int                Moffset;      // private offset of func in vtable
};
typedef struct methodReg *methodReg_t;


struct methodsRegister {
	int               methods_nb;
	methodReg_t     * Register;
};
typedef struct methodsRegister *methodsRegister_t;

inline const char * __class_name_of       (int Oid);
inline const char * __iface_name_of       (int Iid);
inline const char * __method_name_of      (int Mid);

inline int          __interface_id_of     (char *name);
inline int          __method_id_of        (char *name);
inline int          __parent_of           (int Oid);
inline int          __class_idOf          (char *name);
// default toolbox classes builders (build class, and registers methods, not instance constructors)

void __build_object_once        (int);
void __build_scalar_once        (int);
void __build_container_once     (int);
void __build_composite_once     (int);
void __build_string_once        (int);
void __build_raw_once           (int);
void __build_num_once           (int);
void __build_bool_once          (int);
void __build_pointer_once       (int);
void __build_date_once          (int);
void __build_hash_once          (int);
void __build_vector_once        (int);
void __build_dict_once          (int);
void __build_socket_once        (int);
void __build_iterator_once      (int);
void __build_xmldoc_once        (int);
void __build_xmlelt_once        (int);

#endif
