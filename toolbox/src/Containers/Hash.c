/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//=======================================================
// $Id: Hash.c,v 1.2 2004/05/24 16:37:52 plg Exp $
//=======================================================
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
 * @file Hash.c hash tables definition
 */

/** @defgroup Hash Hash_t
 * @ingroup Container
 * Hash specific related functions.
 *
 * Hash tables are one of the simplest and most efficient key/value mapping. This implementation use a well-known and famous hashing functions (see below). 
 * Hash_t objects self-extends the array of lists when they become too deep, insuring a good spread and a relatively homogenous fast access.
 *
 * <b> Hashing function </b> (comments by Ralf S. Engelschall from his 'str' library.)
 *
 * - \b TB_DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 * \par
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp.lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + string[i]''. This is one of the
 * best hashing functions for strings. Because it is both computed very
 * fast and distributes very well.
 * \par
 * The magic of the number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an own RSE-explanation: if one experimentally tests
 * all multipliers between 1 and 256 (as I did it) one detects that
 * even numbers are not useable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%. 
 * \par
 * If one compares the Chi/2 values resulting of the various
 * multipliers, the 33 not even has the best value. But the 33 and a
 * few other equally good values like 17, 31, 63, 127 and 129 have
 * nevertheless a great advantage over the remaining values in the large
 * set of possible multipliers: their multiply operation can be replaced
 * by a faster operation based on just one bit-wise shift plus either a
 * single addition or subtraction operation. And because a hash function
 * has to both distribute good and has to be very fast to compute, those
 * few values should be preferred and seems to be also the reason why
 * Daniel J. Bernstein also preferred it.
 *
 */


#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "tb_global.h"
#include "Toolbox.h"
#include "Hash.h"
#include "tb_ClassBuilder.h"
#include "Memory.h"
#include "Error.h"





Hash_t dbg_tb_hash(char *func, char *file, int line) {
	set_tb_mdbg(func, file, line);
	//	return tb_Hash();
	return tb_hash_new_default();
}

/**
 * Default Hash_t constructor.
 * \ingroup Hash
 * default hash type is KT_STRING (case sensitive), no duplicates
 * @return new Hash_t object
 * \sa Container, tb_HashX
 */
Hash_t tb_Hash() {
	/*
	tb_Object_t (*p)(tb_Object_t, int, int);	
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	p = __getMethod(TB_HASH, OM_NEW);
	return p(tb_newParent(TB_HASH), KT_STRING, 0 );
	*/
	return tb_hash_new_default();
}

Hash_t dbg_tb_hashx(char *func, char *file, int line, int key_type, int allow_duplicates) {
	set_tb_mdbg(func, file, line);
	//	return tb_HashX(key_type, allow_duplicates);
	return tb_HashX(key_type, allow_duplicates);
}

/**
 * Hash_t constructor.
 * \ingroup Hash
 * This implementation of Hash_t constructor allows to choose from any registered key types (see Ktypes.h) and 
 * may allow duplicate values for a given key
 * @param key_type : registered KType 
 * - KT_STRING
 * - KT_ISTRING
 * - KT_INT
 * - KT_POINTER
 * - any other user defined
 * @param allow_duplicates : boolean (true to allow)
 * @return new Hash_t object
 * \sa Container, Ktypes.h
 */
Hash_t tb_HashX(int key_type, int allow_duplicates) {
	/*
	tb_Object_t (*p)(tb_Object_t, int, int);	
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	if( kt_exists(key_type)) {
		p = __getMethod(TB_HASH, OM_NEW);
		return p(tb_newParent(TB_HASH), key_type, allow_duplicates );
	} 
	tb_error("tb_HashX: unknown/unregistered key type %d\n", key_type);
	return NULL;
	*/
	if( kt_exists(key_type)) {
		return tb_hash_new(key_type, allow_duplicates);
	}
	tb_error("tb_HashX: unknown/unregistered key type %d\n", key_type);
	return NULL;
}





/**
 * Get Object_t's key in Hash_t
 *
 * @param H target Hash
 * @param O searched object
 * @return key of Object_t if found else NULL
 *
 * \warning returned value _must_ be used read-only. 
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if H is not a TB_HASH
 *
 * @see Container, tb_HashKeys, tb_getKeybyRef
 * \ingroup Hash
 */
tb_Key_t tb_getKeybyRef(Hash_t H, tb_Object_t O) {
  int i;
  hash_extra_t this;
	int is_a_valid_hash = TB_VALID(H, TB_HASH);

	if(! is_a_valid_hash ) {
		set_tb_errno( TB_ERR_INVALID_TB_OBJECT );
		return KINVAL;
	}

	this = XHASH(H);
  for (i = 0; i < this->size; i++) {
    tb_hash_node_t node, next;
    for (node = this->nodes[i]; node; node = next ) {
			if( node->value == O ) return node->key;
      next = node->next;
    }
  }
	return KINVAL;
}




/**
 * Freeze the auto-resizing feature of Hash when inserting/removing in batch
 *
 * @param H Hash_t target
 * \remarks on error tb_errno will be set to
 * - TB_ERR_INVALID_TB_OBJECT
 *
 * \ingroup Hash
 * @see tb_HashThaw
 */
retcode_t tb_HashFreeze(Hash_t H) {
	no_error;
	if(tb_valid(H, TB_HASH, __FUNCTION__)) {
		XHASH(H)->frozen = 1;

		return TB_OK;
	}
	return TB_ERR;
}

/**
 * Thaw frozen auto-resizing feature of Hash 
 *
 * @param H Hash_t target
 * \remarks on error tb_errno will be set to
 * - TB_ERR_INVALID_TB_OBJECT
 *
 * @see tb_HashFreeze
 * \ingroup Hash
 */
retcode_t tb_HashThaw(Hash_t H) {
	no_error;
	if(tb_valid(H, TB_HASH, __FUNCTION__)) {
		XHASH(H)->frozen = 0;
		return TB_OK;
	}
	return TB_ERR;
}


/**
 * Create a new Vector_t filled with every keys found in Hash
 *
 * Creates a new Vector of keys (tb_Strings)
 *
 * @return new Vector_t 
 *
 * \remarks
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if H not a TB_HASH
 *
 *
 * @see Container, tb_Hash_change_hash_fnc, tb_HashKeys
 * \ingroup Hash
 */
Vector_t tb_HashKeys(Hash_t H) {
  tb_hash_node_t  node;
  int             i = 0;
  Vector_t        V = tb_Vector();
  hash_extra_t    this = XHASH(H);
	k2sz_t          k2sz = kt_getK2sz(this->kt);
	char            buff[20]; // fixme: can overflow

	no_error;
	if(tb_valid(H, TB_HASH, __FUNCTION__)) {
		for (i = 0; i < this->size; i++) {
			for (node = this->nodes[i]; node; node =node->next) {
				tb_Push(V, tb_String("%s", k2sz(node->key, buff)));
			}
		}
		return V;
	}
	return NULL;
}

