/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Ktypes.h,v 1.2 2004/07/01 21:37:01 plg Exp $
//======================================================

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


/* definition/registering of key types for keyed containers, plus misc utility funcs */

#ifndef __KTYPES_H
#define __KTYPES_H


inline tb_Key_t    __cp_int      (tb_Key_t K);
inline tb_Key_t    __cp_str      (tb_Key_t K);

inline int         __cmp_int     (tb_Key_t K1, tb_Key_t K2);
inline int         __cmp_str     (tb_Key_t K1, tb_Key_t K2);
inline int         __cmpi_str    (tb_Key_t K1, tb_Key_t K2);

inline void        __free_int    (tb_Key_t K);
inline void        __free_str    (tb_Key_t K);

inline char *      __strK2sz     (tb_Key_t K, char *buf);
inline char *      __intK2sz     (tb_Key_t K, char *buf);


void register_basic_ktypes_once();


struct ktype {
	int       keytype;
	char    * name;
	kcp_t     kcp;
	kfree_t   kfree;
	kcmp_t    kcmp;
	k2sz_t    k2sz;
};
typedef struct ktype *ktype_t;

struct __global_kt {
	int nb;
	ktype_t *ktypes;
};
typedef struct __global_kt *__global_kt_t;

extern __global_kt_t __GLOBAL_KT;


#endif
