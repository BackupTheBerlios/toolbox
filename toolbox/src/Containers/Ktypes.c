/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Ktypes.c,v 1.1 2004/05/12 22:04:50 plg Exp $
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

/** 
 * @file Ktypes.c definition/registering of key types for keyed containers, plus misc utility funcs 
 */


#include <string.h>
#include <stdio.h>
#include "Toolbox.h"
#include "Ktypes.h"

inline tb_Key_t __cp_int(tb_Key_t K) { return K; }
inline tb_Key_t __cp_str(tb_Key_t K) { return (tb_Key_t)tb_xstrdup(K.key); }

inline int     __cmp_int(tb_Key_t K1, tb_Key_t K2) { 
	return (K1.ndx >= K2.ndx) ? ((K1.ndx==K2.ndx) ? 0 : 1) :-1; }
inline int __cmp_str(tb_Key_t K1, tb_Key_t K2) { return strcmp(K1.key, K2.key); }
inline int __cmp_ptr(tb_Key_t K1, tb_Key_t K2) { 
	return ((unsigned int)K1.user >= (unsigned int)K2.ndx) ? \
		(((unsigned int)K1.ndx==(unsigned int)K2.ndx) ? 0 : 1) :-1; }

inline int __cmpi_str(tb_Key_t K1, tb_Key_t K2) { return strcasecmp(K1.key, K2.key); }

inline void __free_int(tb_Key_t K) { }
inline void __free_str(tb_Key_t K) { tb_xfree(K.key); }

inline char * __strK2sz(tb_Key_t K, char *buf) { sprintf(buf, "%s", K.key); return buf; }
inline char * __intK2sz(tb_Key_t K, char *buf) { sprintf(buf, "%d", K.ndx); return buf; }
inline char * __ptrK2sz(tb_Key_t K, char *buf) { sprintf(buf, "%p", K.user); return buf; }

__global_kt_t __GLOBAL_KT;

/** Called once in classRegistry.c:classRegisterInit()
 *
 * Setup main key types for keyed containers :
 * - KT_STRING : (char *) case sensitive string
 * - KT_ISTRING : (char *) case insensitive string
 * - KT_INT : (int) integer value
 * - KT_POINTER : (void *) user pointer
 */
void register_basic_ktypes_once() {

	__GLOBAL_KT = tb_xcalloc(1, sizeof(struct __global_kt));

	// !!! don't change registering order !!! (some Toolbox.h #define's assume this ordering)
	registerNew_Ktype("KT_STRING",
										__cp_str, __cmp_str, __free_str, __strK2sz);   // char * (case sensitive)
	registerNew_Ktype("KT_ISTRING",
										__cp_str, __cmpi_str, __free_str, __strK2sz);  // char * (case insensitive)
	registerNew_Ktype("KT_INT",
										__cp_int, __cmp_int, __free_int, __intK2sz);   // int
	registerNew_Ktype("KT_POINTER",
										__cp_int, __cmp_ptr, __free_int, __ptrK2sz);   // void *
}




int registerNew_Ktype( char *name, 
											 kcp_t   kcp,
											 kcmp_t  kcmp,
											 kfree_t kfree,
											 k2sz_t k2str) {
	int n = __GLOBAL_KT->nb++;
	ktype_t kt = tb_xcalloc(1, sizeof(struct ktype));
	kt->name    = tb_xstrdup(name);
	kt->keytype = n;
	kt->kcp     = kcp;
	kt->kcmp    = kcmp;
	kt->kfree   = kfree;
	kt->k2sz    = k2str;
	__GLOBAL_KT->ktypes = tb_xrealloc(__GLOBAL_KT->ktypes, __GLOBAL_KT->nb*sizeof(ktype_t));
	__GLOBAL_KT->ktypes[n] = kt;
	return n;
}

inline ktype_t kt_getKtype(int KT) {
	return (KT >=0 && KT <__GLOBAL_KT->nb) ? __GLOBAL_KT->ktypes[KT] : NULL;
}

inline retcode_t kt_exists(int KT) {
	return (KT >=0 && KT <__GLOBAL_KT->nb) ? 1 : 0;
} 

inline kcmp_t kt_getKcmp(int KT) {
	if( kt_exists(KT)) {
		return __GLOBAL_KT->ktypes[KT]->kcmp;
	}
	return NULL;
}

inline kcp_t kt_getKcp(int KT) {
	if( kt_exists(KT)) {
		return __GLOBAL_KT->ktypes[KT]->kcp;
	} 
	return NULL;
}

inline kfree_t kt_getKfree(int KT) {
	if( kt_exists(KT)) {
		return __GLOBAL_KT->ktypes[KT]->kfree;
	} 
	return NULL;
}

inline k2sz_t kt_getK2sz(int KT) {
	if( kt_exists(KT)) {
		return __GLOBAL_KT->ktypes[KT]->k2sz;
	} 
	return NULL;
}

inline const char * kt_getKname(int KT) {
	if( kt_exists(KT)) {
		return __GLOBAL_KT->ktypes[KT]->name;
	} 
	return NULL;
}

inline int       K2i(tb_Key_t K) { return K.ndx;  }
inline char *    K2s(tb_Key_t K) { return K.key;  }
inline void *    K2p(tb_Key_t K) { return K.user; }

inline tb_Key_t  i2K(int    k)   { return (tb_Key_t)k;  }
inline tb_Key_t  s2K(char * k)   { return (tb_Key_t)k;  }
inline tb_Key_t  p2K(void * k)   { return (tb_Key_t)k;  }

