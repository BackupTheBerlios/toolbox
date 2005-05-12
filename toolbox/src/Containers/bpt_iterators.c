//------------------------------------------------------------------
// $Id: bpt_iterators.c,v 1.2 2005/05/12 21:51:08 plg Exp $
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


#include "stdlib.h"

#include "tb_global.h"
#include "Containers.h"
#include "Objects.h"
#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "bplus_tree.h"
#include "iterators.h"


_iterator_ctx_t bpt_newIterCtx(Dict_t D) {
	if( tb_getSize(D) >0) {
		dictIter_t bpi = (dictIter_t)tb_xcalloc(1, sizeof(struct dictIter));
		bpi->Btree = XBpt(D);
		bpi->cur_node = bpi->Btree->first;
		bpi->cur_ndx = 0;
		bpi->hasDupes  = bpi->Btree->dupe_policy;
		bpi->dup_ndx   = 0;
		return (_iterator_ctx_t)bpi;
	}
	return NULL;
}

void bpt_freeIterCtx(Dict_t D) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(D);
	tb_xfree(bpi);
	return;
}

retcode_t bpt_goFirst(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return TB_ERR;
	bpi->cur_node = bpi->Btree->first;
	bpi->cur_ndx = 0;
	bpi->dup_ndx   = 0;
	return TB_OK;
}

retcode_t bpt_find(Iterator_t It, tb_Key_t key) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	int rc;
	if(bpi == NULL) return TB_ERR;
	rc = bpt_lookup(bpi->Btree, key, &(bpi->cur_node), &(bpi->cur_ndx));
	if(0) {
		char tmp[100];
		tb_warn("got to find : <%s>: rc= %d\n", kt_getK2sz(bpi->Btree->kt)(key, tmp),rc);
	}
	return rc;
}


retcode_t bpt_goLast(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return TB_ERR;
	bpi->cur_node  = bpi->Btree->last;
	bpi->cur_ndx   = bpi->cur_node->cnt-1;
	bpi->dup_ndx   = 0;
	return TB_OK;
}

tb_Object_t bpt_goNext(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return NULL;

	if( bpi->hasDupes) {
		if(bpi->cur_node->next == NULL && 
			 bpi->cur_ndx == bpi->cur_node->cnt -1  &&
			 bpi->dup_ndx >= (((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->nb -1)
			 ) return NULL;
	} else {
		if(bpi->cur_node->next == NULL && bpi->cur_ndx == bpi->cur_node->cnt -1) return NULL;
	}

	if( bpi->hasDupes && 
			bpi->dup_ndx < (((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->nb -1) ) { 

		bpi->dup_ndx++;

	} else {
		bpi->dup_ndx = 0;

		if( bpi->cur_ndx <  bpi->cur_node->cnt-1) {
		bpi->cur_ndx++;
	} else {
			if(bpi->cur_node->next) {
		bpi->cur_node = bpi->cur_node->next;
		bpi->cur_ndx=0;
	} else {
				return NULL;
			}
	}
	}


	if( bpi->hasDupes ) {
		// fixme !! check this twice !!
		/*
		if(bpi->cur_node->next == NULL && 
			 bpi->cur_ndx == bpi->cur_node->cnt -1 &&
			 bpi->dup_ndx >= (((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->nb -1)
			 ) return NULL;
		*/
		return (tb_Object_t)((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->values[bpi->dup_ndx];
	}
	//	if(bpi->cur_node->next == NULL && bpi->cur_ndx == bpi->cur_node->cnt -1) return NULL;

	return (tb_Object_t)(bpi->cur_node->ptrs[bpi->cur_ndx]);
}


tb_Object_t bpt_goPrev(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return NULL;

	if( bpi->hasDupes && 
			bpi->dup_ndx < ((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->nb -1 ) { 

		bpi->dup_ndx++;

	} else {
		bpi->dup_ndx = 0;

		if( bpi->cur_ndx > 0) {
			bpi->cur_ndx--;
		} else {
			bpi->cur_node = bpi->cur_node->prev;
			if( bpi->cur_node != NULL) {
				bpi->cur_ndx= bpi->cur_node->cnt-1;
		} else {
			return NULL;
		}
	}
		}
	if( bpi->hasDupes ) {
		return (tb_Object_t)((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->values[bpi->dup_ndx];
	}

	return (tb_Object_t)(bpi->cur_node->ptrs[bpi->cur_ndx]);
}


tb_Key_t bpt_curKey(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return KINVAL;
	if(bpi->cur_node != NULL) {
		return (tb_Key_t)(bpi->cur_node->keys[bpi->cur_ndx]);
	}
	return KINVAL;
}

tb_Object_t bpt_curVal(Iterator_t It) {
	dictIter_t bpi = (dictIter_t)__getIterCtx(It);
	if(bpi == NULL) return NULL;
	if(bpi->cur_node != NULL) {
		if( bpi->hasDupes ) {
			return (tb_Object_t)((bpt_dupes_t)bpi->cur_node->ptrs[bpi->cur_ndx])->values[bpi->dup_ndx];
		}
		return (tb_Object_t) bpi->cur_node->ptrs[bpi->cur_ndx];
	}
	return NULL;
}




// ------> <X> <key>
// greatest lower or equal key
Iterator_t tb_Dict_upperbound(Dict_t D, tb_Key_t key) {
	// fixme: need std valid
	bptree_t Btree =XBpt(D); 
	int n;
	bpt_node_t Node;
	Iterator_t It = NULL;
	dictIter_t bpi;
	
	if( bpt_upper_bound(Btree, key, &Node, &n) == TB_OK) {
		It = tb_Iterator(D);
		bpi = (dictIter_t)__getIterCtx(It);
		bpi->cur_ndx = n;
		bpi->cur_node = Node;
	} else {
		tb_warn("tb_Dict_upperbound: not found\n");
	}
	return It;
}

// <key> <X> <----------
// lowest greater or equal key
Iterator_t tb_Dict_lowerbound(Dict_t D, tb_Key_t key) {
	// fixme: need std valid
	bptree_t Btree =XBpt(D); 
	int n;
	bpt_node_t Node;
	Iterator_t It = NULL;
	dictIter_t bpi;
	
	if( bpt_lower_bound(Btree, key, &Node, &n) == TB_OK) {
		It = tb_Iterator(D);
		bpi = (dictIter_t)__getIterCtx(It);
		bpi->cur_ndx = n;
		bpi->cur_node = Node;
		} else {
			tb_warn("tb_Dict_lowerbound: not found\n");
		}

	return It;
}

