/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: hash_iterators.c,v 1.2 2004/05/24 16:37:52 plg Exp $
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


#include "stdlib.h"

#include "tb_global.h"
#include "Containers.h"
#include "Objects.h"
#include "Toolbox.h"
#include "Hash.h"
#include "Memory.h"
#include "Error.h"
#include "iterators.h"


static int         _findFirst         (Hash_t H, tb_hash_node_t *node, int *ndx);
static int         _findLast          (Hash_t H, tb_hash_node_t *node, int *ndx);
//static hashIter_t  hash_getIterCtx    (Hash_t H);
static hashIter_t  hash_newIterCtx    (Hash_t H);
static void        hash_freeIterCtx   (Hash_t H);
static retcode_t   hash_goFirst       (Iterator_t It);
static retcode_t   hash_goLast        (Iterator_t It);
static tb_Object_t hash_goNext        (Iterator_t It);
static tb_Object_t hash_goPrev        (Iterator_t It);
static tb_Key_t    hash_curKey        (Iterator_t It);
static tb_Object_t hash_curVal        (Iterator_t It);
static retcode_t   hash_find          (Iterator_t It, tb_Key_t key);

// called only once by 'Hash_impl.c:build_hash_once()
// not mandatory, but allows to declare all funcs as 'static'
void register_Hash_Iterable_once(int OID) {
	tb_registerMethod(OID, OM_NEW_ITERATOR_CTX,       hash_newIterCtx);
	tb_registerMethod(OID, OM_FREE_ITERATOR_CTX,      hash_freeIterCtx);
	//	tb_registerMethod(OID, OM_GET_ITERATOR_CTX,       hash_getIterCtx);
	tb_registerMethod(OID, OM_GONEXT,                 hash_goNext);
	tb_registerMethod(OID, OM_GOPREV,                 hash_goPrev);
	tb_registerMethod(OID, OM_GOFIRST,                hash_goFirst);
	tb_registerMethod(OID, OM_GOLAST,                 hash_goLast);
	tb_registerMethod(OID, OM_FIND,                   hash_find);
	tb_registerMethod(OID, OM_CURKEY,                 hash_curKey);
	tb_registerMethod(OID, OM_CURVAL,                 hash_curVal);
}




static int _findFirst(Hash_t H, tb_hash_node_t *node, int *ndx) {
	hash_extra_t members = XHASH(H);
	*ndx=0;
	while( (*node = members->nodes[*ndx]) == NULL && *ndx < members->buckets) { (*ndx)++; 	}
	if(*node) return 1;
	return 0;
}

static int _findLast(Hash_t H, tb_hash_node_t *node, int *ndx) {
	hash_extra_t members = XHASH(H);
	*ndx=members->buckets;

	while( (*node = members->nodes[*ndx]) == NULL &&  *ndx >=0) { 	(*ndx)--; 	}
	if(*node) {
		while((*node)->next) { *node = (*node)->next; }
		return 1;
	}
	return 0;
}


static hashIter_t hash_newIterCtx(Hash_t H) {
	if( tb_getSize(H) >0) {
		hashIter_t hi = (hashIter_t)tb_xcalloc(1, sizeof(struct hashIter));
		hi->H  = H;
		_findFirst(H, &(hi->first), &(hi->first_ndx));
		_findLast(H, &(hi->last), &(hi->last_ndx));
		hi->cur_node   = hi->first;
		hi->cur_ndx    = hi->first_ndx;
		hi->hasDupes   = XHASH(H)->allow_duplicates;
		hi->dup_ndx    = 0;
		return hi;
	}
	return NULL;
}

static void hash_freeIterCtx(Hash_t H) {
	hashIter_t hi = (hashIter_t)__getIterCtx(H);
	if( hi != NULL) tb_xfree(hi);
}

static retcode_t hash_find(Iterator_t It, tb_Key_t key) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	tb_hash_node_t *node;
	if(hi == NULL) return TB_ERR;
  if((node = tb_hash_lookup_node(hi->H, key))) {
		if(*node) {
			hi->cur_node = (*node);
			hi->cur_ndx = 0;
			hi->dup_ndx = 0;
			return TB_OK;
		} else {
			hi->cur_node = NULL;
		}
	}
	return TB_ERR;
}

static retcode_t hash_goFirst(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	if(hi == NULL) return TB_ERR;
	_findFirst(hi->H, &(hi->first), &(hi->first_ndx));
	hi->cur_node = hi->first;
	hi->cur_ndx = hi->first_ndx;
	if(hi->hasDupes) hi->dup_ndx = 0;

	return TB_OK;
}


static retcode_t hash_goLast(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	if(hi == NULL) return TB_ERR;
	_findLast(hi->H, &(hi->last), &(hi->last_ndx));
	hi->cur_ndx = hi->last_ndx;
	hi->cur_node = hi->last;
	if(hi->hasDupes) hi->dup_ndx = hi->last->nb-1;

	return TB_OK;
}


static tb_Object_t hash_goNext(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	hash_extra_t members;
	if(hi == NULL) return NULL;
	
	if(hi->hasDupes && hi->cur_node->nb > ++hi->dup_ndx) {
		return hi->cur_node->values[hi->dup_ndx];
	}

	members = XHASH(hi->H);
	if( hi->cur_node->next) {
		hi->cur_node = hi->cur_node->next;
	} else while( hi->cur_ndx < members->buckets ) {
		hi->cur_ndx++;
		if((hi->cur_node = members->nodes[hi->cur_ndx]) != NULL) break;
	}
	if(hi->hasDupes) {
		if( hi->cur_node) hi->dup_ndx = 0;
		return (hi->cur_node) ? (tb_Object_t) hi->cur_node->values[hi->dup_ndx] : NULL;
	}
	return (hi->cur_node) ? (tb_Object_t) hi->cur_node->value : NULL;
}



static tb_Object_t hash_goPrev(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	hash_extra_t members;
	if(hi == NULL) return NULL;

	if(hi->hasDupes &&  --hi->dup_ndx >=0) {
		return hi->cur_node->values[hi->dup_ndx];
	}

	members = XHASH(hi->H);
	if( hi->cur_node->prev) {
		hi->cur_node = hi->cur_node->prev;
	} else while( hi->cur_ndx > 0 ) {
		hi->cur_ndx--;
		if((hi->cur_node = members->nodes[hi->cur_ndx]) != NULL) {
			while( hi->cur_node->next ) { 
				hi->cur_node = hi->cur_node->next; 
			} 
			break;
		}
	}
	if(hi->hasDupes) {
		if(hi->cur_node) hi->dup_ndx = hi->cur_node->nb-1;
		return (hi->cur_node) ? (tb_Object_t) hi->cur_node->values[hi->dup_ndx] : NULL;
	}
	return (hi->cur_node) ? (tb_Object_t) hi->cur_node->value : NULL;
}


static tb_Key_t hash_curKey(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	return (hi->cur_node) ? hi->cur_node->key : KINVAL;
}


static tb_Object_t hash_curVal(Iterator_t It) {
	hashIter_t hi = (hashIter_t)__getIterCtx(It);
	if(hi->hasDupes) {
		return (hi->cur_node) ? (tb_Object_t) hi->cur_node->values[hi->dup_ndx] : NULL;
	}
	return (hi->cur_node) ? (tb_Object_t) hi->cur_node->value : NULL;
}



