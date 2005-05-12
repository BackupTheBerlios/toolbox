//===================================================
// $Id: bplus_tree.c,v 1.3 2005/05/12 21:51:08 plg Exp $
//===================================================
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

/* TODO:  */
/*  + separate static & private methods from public exported ones (cf Hash) */
/*  + fix remove_key sticky bug */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Toolbox.h"
#include "tb_global.h"
#include "bplus_tree.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"

#include "Memory.h"
#include "Error.h"

inline bptree_t XBpt(Dict_t This) { 
	return (bptree_t)((dict_extra_t)
										((__members_t)
										 tb_getMembers(This, TB_DICT))->instance)->BplusTree;
}

inline static tb_Key_t     bpt_get_node_key    (bpt_node_t n, int ndx);
inline static bpt_node_t   bpt_get_node_ptr    (bpt_node_t n, int ndx);
static int                 bpt_node_insert     (bptree_t Btree, bpt_node_t node, tb_Key_t key, void *ptr);
static void                bpt_node_split      (bptree_t Btree, bpt_node_t dst, bpt_node_t src);
static int                 bpt_scatter_gather  (bptree_t Btree, bpt_node_t node, tb_Key_t key, void *ptr);
static bpt_node_t          bpt_newNode         (bptree_t Btree);
static bpt_leaf_t          bpt_newLeaf         (bptree_t Btree);

static bpt_node_t          bpt_remove_key      (bptree_t Tree, bpt_node_t node, tb_Key_t key, bpt_node_t ptr);
static int                 bpt_dicho_search    (bpt_node_t node, int start, int end, 
																								kcmp_t cmp, tb_Key_t key);
static int                 bpt_dicho_search_interval (bpt_node_t node, int start, int end, 
																											kcmp_t cmp, tb_Key_t key);
static int                 bpt_add             (bptree_t Btree, tb_Key_t key, void *value, int allow_replace);
static int                 bpt_change_key      (bpt_node_t node, tb_Key_t oldkey, tb_Key_t newkey, 
																								int kt);

static bptree_t            bpt_newTree         (int branch_factor, int allow_dupes, int  kt);
static void                dump_tree_uniq      (bpt_node_t N, int level, int kt);
static void                dump_tree_mult      (bpt_node_t N, int level, int kt);


void __build_dict_once(int OID) {
	tb_registerMethod(OID, OM_NEW,                    tb_dict_new_default);
	tb_registerMethod(OID, OM_FREE,                   tb_dict_free);
	tb_registerMethod(OID, OM_GETSIZE,                tb_dict_getsize);
	//CODE-ME:	registerNewMethod(TB_DICT, OM_CLONE,        tb_dict_clone);
	tb_registerMethod(OID, OM_DUMP,                   tb_dict_dump);
	tb_registerMethod(OID, OM_CLEAR,                  tb_dict_clear);
	//CODE-ME:	registerNewMethod(TB_DICT, OM_SERIALIZE,    tb_dict_marshall);
	//CODE-ME:	registerNewMethod(TB_DICT, OM_UNSERIALIZE,  tb_dict_unmarshall);
	tb_registerMethod(OID, OM_REPLACE,                tb_dict_replace);
	tb_registerMethod(OID, OM_INSERT,                 tb_dict_insert);
	tb_registerMethod(OID, OM_EXISTS,                 tb_dict_exists);
	tb_registerMethod(OID, OM_GET,                    tb_dict_get);
	tb_registerMethod(OID, OM_TAKE,                   tb_dict_take);
	tb_registerMethod(OID, OM_REMOVE,                 tb_dict_remove);
	
	tb_registerMethod(OID, OM_NEW_ITERATOR_CTX,       bpt_newIterCtx);
	tb_registerMethod(OID, OM_FREE_ITERATOR_CTX,      bpt_freeIterCtx);
	//	tb_registerMethod(OID, OM_GET_ITERATOR_CTX,       bpt_getIterCtx);
	tb_registerMethod(OID, OM_GONEXT,                 bpt_goNext);
	tb_registerMethod(OID, OM_GOPREV,                 bpt_goPrev);
	tb_registerMethod(OID, OM_FIND,                   bpt_find);
	tb_registerMethod(OID, OM_GOFIRST,                bpt_goFirst);
	tb_registerMethod(OID, OM_GOLAST,                 bpt_goLast);
	tb_registerMethod(OID, OM_CURKEY,                 bpt_curKey);
	tb_registerMethod(OID, OM_CURVAL,                 bpt_curVal);
}

Dict_t tb_dict_new_default() {
	return tb_dict_new(KT_STRING, 0);
}

Dict_t tb_dict_new(int keytype, int allow_dupes) {
	Dict_t O;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);

	if(kt_exists(keytype)) {

		O = tb_newParent(TB_DICT);

		O->isA   = TB_DICT;
		O->members->instance            = tb_xcalloc(1, sizeof(struct dict_extra));
		((struct dict_extra *)O->members->instance)->BplusTree = bpt_newTree(10, allow_dupes, keytype);

		if( fm->dbg) fm_addObject(O);

		return O;
	}
	tb_error("unknown/unregistered key type %d\n", keytype);
	return NULL;
}


inline bpt_node_t bpt_lookup_node(bptree_t Btree, tb_Key_t key) {

	int i, n;
	bpt_node_t root = Btree->root;
	bpt_node_t node = root;
	kcmp_t cmp   = kt_getKcmp(Btree->kt);


	while( node && node->type != BPT_LEAF ) {
		n = node->cnt;

		if( cmp(key, bpt_get_node_key(node, 0)) <= 0) {
			node = bpt_get_node_ptr(node, 0);
		} else if( cmp(key, bpt_get_node_key(node, n-1)) > 0) {
			node = bpt_get_node_ptr(node, n);
		} else {

			i = bpt_dicho_search_interval(node, 1, n, cmp, key);
			if(i == -1) return NULL;
			node = bpt_get_node_ptr(node, i);
		}
	}
	return node;
}


int tb_dict_getsize(Dict_t D) {
	return XBpt(D)->size;
}

inline static int bpt_search_ptr(bpt_node_t node, kcmp_t cmp, tb_Key_t key, void *ptr) {
	int n, i;

	n = node->cnt;

	if( cmp(key, bpt_get_node_key(node, 0)) <= 0) {
		return 0;
	} else if( cmp(key, bpt_get_node_key(node, n-1)) > 0) {
		return n;
	} 
	i = bpt_dicho_search_interval(node, 1, n, cmp, key);
	return i;
}


inline static tb_Key_t bpt_get_node_key(bpt_node_t n, int ndx) {
	if(n == NULL) {
		tb_error("node is null !!!\n");
		return KINVAL;
	}
	if(ndx > n->cnt -1) {
		tb_error("pnk: index out of bounds !!\n");
		return KINVAL;
	}
	return (tb_Key_t)n->keys[ndx];
}

inline static bpt_node_t bpt_get_node_ptr(bpt_node_t n, int ndx) {
	if(n == NULL) {
		tb_error("node is null !!!\n");
		return NULL;
	}
	if(ndx > n->cnt ) {
		tb_error("pnp: index out of bounds !!\n");
		return NULL;
	}
	return (bpt_node_t)n->ptrs[ndx];
}


static bpt_node_t bpt_newNode(bptree_t Btree) {
	bpt_node_t N = tb_xcalloc(1, sizeof(struct bptree_node));
	N->keys  =  tb_xcalloc(Btree->branch_factor, sizeof(void *));
	N->ptrs  =  tb_xcalloc(Btree->branch_factor, sizeof(void *));
	N->type  = BPT_NODE;
	return N;
}

static bpt_leaf_t bpt_newLeaf(bptree_t Btree) {
	bpt_leaf_t leaf  = bpt_newNode(Btree);
	leaf->type   = BPT_LEAF;
	return leaf;
}
 
static void bpt_free_node(bpt_node_t node, kfree_t kfree) {
	int i;
	for(i=0; i<node->cnt; i++) {
		kfree(bpt_get_node_key(node, i));
	}
	tb_xfree(node->keys);
	tb_xfree(node->ptrs);
	tb_xfree(node);
}




static int bpt_scatter_gather(bptree_t Btree, bpt_node_t node, tb_Key_t key, void *ptr) {
	if(node == NULL) return 1;

	bpt_node_insert(Btree, node, key, ptr);

	if( node->cnt >= Btree->branch_factor ) { // dest node is full
		bpt_node_t new = bpt_newNode(Btree);
		bpt_node_split(Btree, new, node);

		if(node->parent == NULL) {
			Btree->root  = bpt_newNode(Btree);
			node->parent = Btree->root; 
			node->parent->ptrs[0] = node;

			node->parent->keys[0] = kt_getKcp(Btree->kt)( bpt_get_node_key(node, node->cnt-1));
			node->parent->ptrs[1] = new;
			node->parent->cnt++;

			node->parent = new->parent = Btree->root;

			return 1;
		} 

		new->parent = node->parent;
		bpt_scatter_gather(Btree, 
											 node->parent, 
											 bpt_get_node_key(node, node->cnt-1),
											 new); 
	}

	return 1;
}


// vec is of size [BRANCH_FACTOR +1]
static int bpt_node_insert(bptree_t Btree, bpt_node_t node, tb_Key_t key, void *ptr) {
	int i = 0, n = node->cnt, dupe = 0;
	int rc;
	int klen, plen;
	kcmp_t cmp   = kt_getKcmp(Btree->kt);
	kcp_t cp = kt_getKcp(Btree->kt);

	if(node->type == BPT_NODE) {
		((bpt_node_t)ptr)->parent = node;
	}

	if( n >0 ) {
		for(i=0;i<n;i++) { // FIXME: use dicho search 
			rc = cmp( key, node->keys[i] );
			if( rc <=0 ) {
				if( rc == 0 ) {
					if(Btree->dupe_policy == 0 ) {
						tb_warn("!! <%s> already stored !!\n", key);  
						return TB_KO; 
					} else {
						dupe = 1;
					}
				}
				break;
			}
		}
	}

	if( ! dupe ) {

		if( node->cnt > 0 && node->cnt >= i) {
			klen  = sizeof(tb_Key_t) * (n-i);
			plen  = sizeof(void *) * (n-i);
			memmove(node->keys +i+1, node->keys +i, klen );
			if( node->type == BPT_NODE) {
				memmove(node->ptrs +i+1+1, node->ptrs +i+1, plen );
			} else {
				memmove(node->ptrs +i+1, node->ptrs +i, plen );
			}
		}
		node->keys[i] = cp(key);
		if( node->type == BPT_NODE) { i++; } 
		node->cnt ++;

	}



	if( node->type == BPT_LEAF ) {

		if( Btree->dupe_policy == 0 ) {
			node->ptrs[i] = ptr;
		} else {
			bpt_dupes_t d;
			if(! dupe ) {
				d = tb_xcalloc(1, sizeof(struct bpt_dupes));
				d->nb = 0;
				node->ptrs[i] = d;
			} else {
				d = node->ptrs[i];
			}
			d->nb++;
			d->values = tb_xrealloc(d->values, sizeof(void *) * d->nb);
			d->values[d->nb-1] = ptr;
		}

	} else { // == BPT_NODE

		node->ptrs[i] = ptr;
		
		if( i > 0 ) {
			((bpt_node_t)ptr)->prev = node->ptrs[ i-1 ];
			((bpt_node_t)node->ptrs[ i-1 ])->next = ptr;
		}
		if( i < node->cnt-1 ) {
			((bpt_node_t)ptr)->next = node->ptrs[ i+1 ];
			((bpt_node_t)node->ptrs[ i+1 ])->prev = ptr;
		}
	}

	return TB_OK;
}



static void bpt_node_split(bptree_t Btree, bpt_node_t dst, bpt_node_t src) { 
	int i, n = src->cnt /2;
	int nb = src->cnt -n;
	int klen = sizeof(tb_Key_t) * nb;
	int plen = sizeof(void *) * nb;

	src->cnt -=nb;
	dst->cnt +=nb;

	if( dst->type == BPT_LEAF ) {
		memcpy(dst->keys, src->keys +n, klen);
		memset(src->keys +n, 0, klen);

		memmove(dst->ptrs, src->ptrs +n, plen);
		memset(src->ptrs +n, 0, plen);

	} else { // => BPT_NODE

		memcpy(dst->keys, src->keys +n, klen);
		memset(src->keys +n, 0, klen);

		memmove(dst->ptrs , src->ptrs +n, plen+sizeof(void *));
		memset(src->ptrs +n, 0, plen);

		for(i=0; i<nb+1; i++) {// fixme : optimizable
			if(dst->ptrs[i] != NULL) {
				((bpt_node_t)dst->ptrs[i])->parent = dst;
			}
		}
	}
}

	
	


static int bpt_change_key(bpt_node_t node, tb_Key_t oldkey, tb_Key_t newkey, 
													int kt) {
	int k;
	kcmp_t   cmp =    kt_getKcmp(kt);
	kcp_t    cp =     kt_getKcp(kt);
	kfree_t  kfree =  kt_getKfree(kt);

	k = bpt_dicho_search(node, 0, node->cnt, cmp, oldkey); 
	if( k == -1 ) {
		return -1;
	}

	kfree(bpt_get_node_key(node, k));
	node->keys[k] = cp(newkey);
	if( node->parent ) {
		return bpt_change_key(node->parent, oldkey, newkey, kt);
	}

	return TB_OK;
}

static int bpt_merge_nodes(bpt_node_t left, bpt_node_t right, int kt) {
	//insert left in right, and destroy left
	int i;
	int l_sz = left->cnt;
	int r_sz = right->cnt;
	tb_Key_t *l_kvec = left->keys;
	void **l_pvec = left->ptrs;
	tb_Key_t *r_kvec = right->keys;
	void **r_pvec = right->ptrs;

	assert(left != right);

	// shift right data 
	memmove(r_kvec +l_sz, r_kvec, r_sz * sizeof(tb_Key_t));
	memmove(r_pvec +l_sz, r_pvec, (r_sz+1) * sizeof(void *));
	// cpy left data to right
	memcpy(r_kvec, l_kvec, l_sz * sizeof(tb_Key_t));
	memcpy(r_pvec, l_pvec, l_sz * sizeof(void *));

	right->cnt += left->cnt;
	left->cnt =0;
	if( right->type == BPT_NODE) {
		for(i=0; i<right->cnt; i++) {
			((bpt_node_t)right->ptrs[i])->parent = right;
		}
		if( ((bpt_node_t)right->ptrs[i]) ) {
			((bpt_node_t)right->ptrs[i])->parent = right;
		}
	}

	right->prev = left->prev;
	if(left->prev) left->prev->next = right;

	bpt_free_node(left, kt_getKfree(kt));

	return 1;
}

static bpt_node_t bpt_remove_key(bptree_t Tree, bpt_node_t node, tb_Key_t key,
																 bpt_node_t ptr) {
	bpt_node_t parent;
	tb_Key_t ref_key;
	int k;
	kcmp_t    cmp = kt_getKcmp(Tree->kt);
	kfree_t kfree = kt_getKfree(Tree->kt);
	int underflow = Tree->branch_factor;


	if( node == NULL ) return NULL;

	//	tb_warn("bpt_remove_key: in node <%p>\n", node);
	parent = node->parent;


	ref_key = bpt_get_node_key(node, node->cnt-1);
	k = bpt_dicho_search(node, 0, node->cnt, cmp, key); 


	if( k == -1 ) {
		if( node->type == BPT_NODE ) { // node may be referenced by last parent ptr

			// FIXME: a bug is creeping in the dark just after the corner -->

			/*
			if( parent && (bpt_get_node_ptr(parent, parent->cnt) == ptr)) {
				tb_notice("freeing node %p !\n", parent->ptrs[parent->cnt]);
				bpt_free_node(parent->ptrs[parent->cnt], Tree->ktype->kfree);
				parent->ptrs[parent->cnt] = 0;
			}
			*/
			if( parent ) {
				if(bpt_get_node_ptr(parent, parent->cnt) == ptr) {
					//					tb_warn("freeing node %p !\n", parent->ptrs[parent->cnt]);
					bpt_free_node(parent->ptrs[parent->cnt], kfree);
					parent->ptrs[parent->cnt] = 0;
				}
			} else {
				if(node->ptrs[node->cnt] == ptr) {
					//					tb_warn("freeing node %p in root!\n", node->ptrs[node->cnt]);
					bpt_free_node(ptr, kfree);
					node->ptrs[node->cnt] = 0;
				}
			}
		}
		return node;
	}

	if( node->cnt == 1 ) { // remove whole node 

		//		tb_warn("bpt_remove_key: removing whole node\n");

		if(node == Tree->root) {
			Tree->root = Tree->first = Tree->last = NULL;
		}
		// freeing is done in parent when unwinding the stack

		if(node->type == BPT_LEAF) {
			if(node->next) {
				node->next->prev = node->prev;
			} else {
				Tree->last = node->prev;
			}
			if(node->prev) {
				node->prev->next = node->next;
			} else {
				Tree->first = node->next;
			}
		}
		bpt_free_node(node, kfree);

		return bpt_remove_key(Tree, parent, key, node);
	} 

	kfree(bpt_get_node_key(node, k));

	if( k == node->cnt-1) {
		node->keys[k] = (tb_Key_t)0;
		if( node->type == BPT_LEAF ) {
			node->ptrs[node->cnt-1] = 0;
		} else {
			node->ptrs[node->cnt-1] = node->ptrs[node->cnt];
			node->ptrs[node->cnt] = 0;
		}
	} else {
		if( node->type == BPT_LEAF ) {
			memmove(node->ptrs +k, node->ptrs +k+1, sizeof(void *) * (node->cnt -k));
			node->ptrs[node->cnt-1] = 0;
		} else {
			memmove(node->ptrs +k, node->ptrs +k+1, sizeof(void *) * (node->cnt+1 -k));
			node->ptrs[node->cnt] = 0;
		}

		memmove(node->keys +k, node->keys +k+1, sizeof(tb_Key_t) * (node->cnt -k));
		node->keys[node->cnt-1] = (tb_Key_t)0;
	}
	node->cnt--;

	if( k == node->cnt ) { //if last key was removed, must be changed in parent
		if( parent ) {
			bpt_change_key(parent, key, node->keys[k-1], Tree->kt);
			return node;
		}
	}

	if( parent ) {
		// find if prev or next sibling is also underflowed to merge both
		int pk = bpt_search_ptr(parent, cmp, ref_key, node);

		if( pk > 0 ) {
			void *lefty = parent->ptrs[pk-1];
			int l_sibling_sz = ((bpt_node_t)lefty)->cnt;

			if( node->cnt + l_sibling_sz < underflow) {
				// destroy ref to sibling in parent
				node->parent = bpt_remove_key(Tree, parent, 
																			bpt_get_node_key( lefty, l_sibling_sz -1), lefty);
				
				if( node->parent != NULL ) { 
					if(k == node->cnt) {
						bpt_change_key(node->parent, ref_key,  node->keys[k-1], Tree->kt);
					} 
				} else {
					Tree->root = node;
				}
				tb_debug("left-merge %p - %p\n", lefty, node);
				bpt_merge_nodes(lefty, node, Tree->kt);
				if(node->type == BPT_LEAF && node->prev == NULL) Tree->first = node;

				return node;
			}

		} else if( pk < parent->cnt-1 ) {
			void *righty = parent->ptrs[pk+1];
			int r_sibling_sz = ((bpt_node_t)parent->ptrs[pk+1])->cnt;

			if( node->cnt + r_sibling_sz < underflow) {
				// destroy ref to node in parent
				((bpt_node_t)righty)->parent = bpt_remove_key(Tree, parent, ref_key, node);
				if( ((bpt_node_t)righty)->parent == NULL) {
					Tree->root = righty;
				}
				tb_debug("right-merge %p - %p\n", node, righty);
				bpt_merge_nodes(node, righty, Tree->kt);
				if(((bpt_node_t)righty)->type == BPT_LEAF && 
					 ((bpt_node_t)righty)->prev == NULL) Tree->first = righty;

				return righty;
			}
		}
	}
	

	return node;
}


static int bpt_dicho_search(bpt_node_t node, int start, int end, kcmp_t cmp, tb_Key_t key) {
	int cmp_rc, mid, len = end - start;
	tb_Key_t *vec = node->keys;

	if( len < 0 ) return -1;

	mid = len/2 +start;
	cmp_rc = cmp(key, vec[mid]);
	if(cmp_rc > 0) { 
		if( mid == node->cnt-1) return -1;
		return bpt_dicho_search(node, mid+1, end, cmp, key);
	} else if(cmp_rc < 0){ 
		if( mid == 0) return -1;
		return bpt_dicho_search(node, start, mid-1, cmp, key);
	}
	return mid; 
}



static int bpt_dicho_search_interval(bpt_node_t node, int start, int end, 
																		 kcmp_t cmp, tb_Key_t key) {
	int cmp_rc1, cmp_rc2, mid, len = end - start;
	tb_Key_t *vec = node->keys;

	if( len < 1 ) return -1;

	mid = len/2 +start;

	cmp_rc1 = cmp(key, vec[mid-1]);
	cmp_rc2 = cmp(key, vec[mid]);

	if(cmp_rc2 <= 0) {

		if(cmp_rc1 > 0) {
			return mid;
		}
 		return bpt_dicho_search_interval(node, start, mid, cmp, key);
	}
	return bpt_dicho_search_interval(node, mid, end, cmp, key);
}




// public

static bptree_t bpt_newTree(int branch_factor, int allow_dupes, int keytype) {
	bptree_t Btree         = tb_xcalloc(1, sizeof(struct bptree));
	Btree->kt              = keytype;
	Btree->branch_factor   = branch_factor;
	Btree->dupe_policy     = allow_dupes;
	Btree->root            = bpt_newNode( Btree );

	return Btree;
}

tb_Object_t tb_dict_get(Dict_t D, tb_Key_t key) {
	bptree_t Btree = XBpt(D);
	void *ptr   = NULL;
	bpt_node_t node;
	int n;
	if(Btree->cnt == 0) return NULL;

	node = bpt_lookup_node(Btree, key);

	if(node != NULL) {
		n = bpt_dicho_search(node, 0, node->cnt, kt_getKcmp(Btree->kt), key);
		if( n != -1) {
			ptr = bpt_get_node_ptr(node, n);
		}
	}

	return (tb_Object_t)ptr;
}


retcode_t bpt_lookup(bptree_t Btree, tb_Key_t key, 	bpt_node_t *node, int *n) {
	if(Btree->cnt == 0) return TB_KO;

	*node = bpt_lookup_node(Btree, key);

	if(*node != NULL) {
		*n = bpt_dicho_search(*node, 0, (*node)->cnt, kt_getKcmp(Btree->kt), key);
		if( *n != -1) return TB_OK;
	}
	return TB_KO;
}

int tb_dict_exists(Dict_t D, tb_Key_t key) {
	bptree_t Btree = XBpt(D);
	bpt_node_t node;
	int n;
	if(Btree->cnt == 0) return 0;

	node = bpt_lookup_node(Btree, key);
	if(node != NULL) {
		n = bpt_dicho_search(node, 0, node->cnt, kt_getKcmp(Btree->kt), key);
		if( n != -1) {
			if( Btree->dupe_policy ) {
				bpt_dupes_t d = node->ptrs[n];
				return d->nb;
			} else {
				return 1;
			}
		}
	}

	return 0;
}

// if duplicates, extract only one
tb_Object_t tb_dict_take(Dict_t D, tb_Key_t key) {
	bptree_t Btree = XBpt(D);
	void *ptr   = NULL;
	bpt_node_t node;
	int n;
	if(Btree->cnt == 0) return NULL;

	node = bpt_lookup_node(Btree, key);

	if(node != NULL) {
		n = bpt_dicho_search(node, 0, node->cnt, kt_getKcmp(Btree->kt), key);
		if( n != -1) {

			if( Btree->dupe_policy == 0 ) {
				ptr = bpt_get_node_ptr(node, n);
				bpt_remove_key(Btree, node, key, node);
				TB_UNDOCK((tb_Object_t)ptr);
				Btree->cnt--;
				Btree->size --;
			} else { // duplicates allowed
				bpt_dupes_t d = (bpt_dupes_t)bpt_get_node_ptr(node, n);
				d->nb--;
				ptr = d->values[d->nb];
				TB_UNDOCK((tb_Object_t)ptr);
				Btree->cnt--;
	
				if(d->nb >0) {
					d->values = tb_xrealloc(d->values, sizeof(void *) * d->nb);
				} else {

					tb_warn("tb_dict_take: dupe box empty< %p >, removing key\n", d);

					bpt_remove_key(Btree, node, key, node);
					Btree->size --;
					tb_xfree(d->values);
					tb_xfree(d);
				}
			}		
		}
	}

	return (tb_Object_t)ptr;
}

int tb_dict_remove(Dict_t D, tb_Key_t key) {
	bptree_t Btree = XBpt(D);
	void *ptr   = NULL;
	bpt_node_t node;
	int n;
	if(Btree->cnt == 0) return TB_KO;

	node = bpt_lookup_node(Btree, key);

	if(node != NULL) {
		n = bpt_dicho_search(node, 0, node->cnt, kt_getKcmp(Btree->kt), key);
		if( n != -1) {
			if( Btree->dupe_policy ) {
				int d;
				bpt_dupes_t dup = (bpt_dupes_t)node->ptrs[n];

				fm_fastfree_on();

				for(d=0; d < dup->nb; d++) {
					TB_UNDOCK((tb_Object_t)dup->values[d]);
					tb_Free(dup->values[d]);
					Btree->cnt--;
				}
				bpt_remove_key(Btree, node, key, node);
				tb_xfree(dup->values);
				tb_xfree(dup);
				fm_fastfree_off();
			} else {
				ptr = bpt_get_node_ptr(node, n);
				bpt_remove_key(Btree, node, key, node);
				TB_UNDOCK((tb_Object_t)ptr);
				tb_Free((tb_Object_t)ptr);
				Btree->cnt--;
			}
			Btree->size --;
			return TB_OK;
		}
	}
	return TB_KO;
}




/** Search for greatest key <= key
 * @ingroup Dict
 */
int bpt_upper_bound(bptree_t Btree, tb_Key_t key, bpt_node_t *Node, int *Ndx) {
	int i, n = 0;
	bpt_node_t root = Btree->root;
	bpt_node_t node = root;
	kcmp_t cmp   = kt_getKcmp(Btree->kt);


	while( node && node->type != BPT_LEAF ) {
		n = node->cnt;

		if( cmp(key, bpt_get_node_key(node, 0)) <= 0) {
			node = bpt_get_node_ptr(node, 0);
		} else if( cmp(key, bpt_get_node_key(node, n-1)) > 0) {
			node = bpt_get_node_ptr(node, n);
		} else {

			i = bpt_dicho_search_interval(node, 1, n, cmp, key);
			if(i == -1) {
				return TB_ERR;
			}
			node = bpt_get_node_ptr(node, i);
		}
	}

	if( cmp(bpt_get_node_key(node, 0), key) > 0) {
		if( node == Btree->first) {
			return TB_ERR;
		}
		node = node->prev;
	}


	for(n=node->cnt-1; n>=0; n--) {
		int rc = cmp(bpt_get_node_key(node, n), key);
		if(rc <= 0) {
			*Node = node;
			*Ndx = n;
			return TB_OK;
		}
	}
	return TB_ERR;
}

/** Search for lowest key >= key
 * @ingroup Dict
 */
int  bpt_lower_bound(bptree_t Btree, tb_Key_t key, bpt_node_t *Node, int *Ndx) {
	int i, n =0;
	bpt_node_t root = Btree->root;
	bpt_node_t node = root;
	kcmp_t cmp   = kt_getKcmp(Btree->kt);


	while( node && node->type != BPT_LEAF ) {
		n = node->cnt;

		if( cmp(key, bpt_get_node_key(node, 0)) <= 0) {
			node = bpt_get_node_ptr(node, 0);
		} else if( cmp(key, bpt_get_node_key(node, n-1)) > 0) {
			node = bpt_get_node_ptr(node, n);
		} else {

			i = bpt_dicho_search_interval(node, 1, n, cmp, key);
			if(i == -1) {
				return TB_ERR;
			}
			node = bpt_get_node_ptr(node, i);
		}
	}


	if( cmp(bpt_get_node_key(node, node->cnt-1), key) < 0) {
		if( node == Btree->last) return TB_ERR;
		node = node->next;
	}

	for(n=0; n<node->cnt; n++) {
		int rc = cmp(bpt_get_node_key(node, n), key);
		if(rc >= 0) {
			*Node = node;
			*Ndx = n;
			return TB_OK;
		}
	}

	return TB_ERR;
}


int tb_dict_insert(Dict_t Dict, void *value, tb_Key_t key) {
	int rc = bpt_add(XBpt(Dict), key, value, 0);
	if( rc == TB_OK) XBpt(Dict)->size++;
	return rc;
}


int tb_dict_replace(Dict_t Dict, void *value, tb_Key_t key) {
	int rc;
	bptree_t Btree = XBpt(Dict);
	// if duplicates allowed, wipe olds out first
	if(Btree->dupe_policy ) tb_dict_remove(Dict, key);
	rc = bpt_add(Btree, key, value, 1);
	if( rc == TB_OK) Btree->size++;
	return rc;
}

static int bpt_add(bptree_t Btree, tb_Key_t key, void *value, int allow_replace) {

	int i, n;
	bpt_leaf_t leaf = NULL;
	bpt_node_t root = Btree->root;
	bpt_node_t prev = NULL;
	bpt_node_t node = root;
	kcmp_t cmp   = kt_getKcmp(Btree->kt);
	int rc = TB_ERR;


	TB_DOCK((tb_Object_t)value);

	if( root->cnt == 0 ) { // fresh empty tree
		leaf = bpt_newLeaf(Btree);
		leaf->parent = root;
		rc = bpt_node_insert(Btree, leaf, key, value);
		root->keys[0] = kt_getKcp(Btree->kt)(key);
		root->ptrs[0] = leaf;

		root->cnt ++;
		Btree->cnt++;
		Btree->first = Btree->last = leaf;

		return rc;
	}

	while( node && node->type != BPT_LEAF ) {
		prev = node;
		n = node->cnt;

		if( cmp(key, bpt_get_node_key(node, 0)) <= 0) {
			node = bpt_get_node_ptr(node, 0);
		} else if( cmp(key, bpt_get_node_key(node, n-1)) > 0) {
			node = bpt_get_node_ptr(node, n);
			if( node == NULL ) {
				bpt_leaf_t leaf = bpt_newLeaf(Btree);
				prev->ptrs[n] = leaf;
				rc = bpt_node_insert(Btree, prev->ptrs[n], key, value);

				leaf->parent = prev;
				leaf->next = ((bpt_leaf_t)prev->ptrs[n-1])->next;

				((bpt_leaf_t)prev->ptrs[n-1])->next = leaf;
				leaf->prev = ((bpt_leaf_t)prev->ptrs[n-1]);
				if( leaf->next ) {
					leaf->next->prev = leaf;
				} else {
					Btree->last = leaf;
				}

				Btree->cnt++;
				return rc;
			}
		} else {
			i = bpt_dicho_search_interval(node, 1, n, cmp, key);
			if(i == -1) return TB_KO;
			node = bpt_get_node_ptr(node, i);
		}
	}

	if( node != NULL ) {
		int ndx;
		leaf = (bpt_leaf_t)node;

		if( (ndx = bpt_dicho_search(node, 0, leaf->cnt, cmp, key)) != -1) { // found in leaf
			switch( Btree->dupe_policy ) {
			case 0: // no dupes allowed
				if( allow_replace ) {
					//					tb_warn("key overwritten\n");
					TB_UNDOCK(node->ptrs[ndx]);
					tb_Free(node->ptrs[ndx]);
					node->ptrs[ndx] = value; 
					return TB_OK;
				}
				tb_warn("bpt_add: key<%s> already exists !\n", key);
				TB_UNDOCK((tb_Object_t)value);
				return TB_KO;
			case 1: // dupes welcomes
					return bpt_node_insert(Btree, leaf, key, value);
				} 

		} else { // not found

			if( leaf->cnt < Btree->branch_factor-1 ) { // leaf is not full
				rc = bpt_node_insert(Btree, leaf, key, value);

			} else { // leaf is full : need to split

				bpt_leaf_t L = bpt_newLeaf(Btree);

				// insert key in correct pos in full leaf (real size is full +1)
				rc = bpt_node_insert(Btree, leaf, key, value);
				bpt_node_split(Btree, L, leaf);


				// now we have two half-leafs
				// reorg leafs linkage

				L->next = leaf->next;
				L->prev = leaf;
				leaf->next   = L;
				if( L->next ) {
					L->next->prev = L;
				} else {
					Btree->last = L;
				}

				L->parent = prev;

				bpt_scatter_gather(Btree, prev, bpt_get_node_key(leaf, leaf->cnt-1), L); 
			}
		}

	} else {	// no leaf
		leaf = bpt_newLeaf(Btree);
		rc = bpt_node_insert(Btree, leaf, key, value);
		leaf->parent = prev;
		bpt_scatter_gather(Btree, leaf->parent, bpt_get_node_key(leaf,0), leaf);
 	}
	Btree->cnt ++;

	return rc;
}


void tb_dict_dump(Dict_t D, int level) {
	bptree_t Btree = XBpt(D);
	if(Btree->dupe_policy ) {
		dump_tree_mult(Btree->root, level, Btree->kt);
	} else {
		dump_tree_uniq(Btree->root, level, Btree->kt);
	}
}

static void dump_tree_uniq(bpt_node_t N, int level, int kt) {
	int i;
	FILE *out = stderr;
	k2sz_t k2sz = kt_getK2sz(kt);
	char buff[100];

	if( level == 0 ) {
		fprintf(out, "-----------------------------\n");
	}
	for(i=0; i<level; i++) {
		fprintf(out, "|");
	}
	if(N) {
	if( N->type == BPT_NODE ) {
		fprintf(out, "+ Node[%p:%d]::%p : %p", N, N->cnt, N->parent, N->ptrs[0]);
		for(i=0; i<N->cnt; i++) {
			fprintf(out, " <= %s < %p", k2sz(N->keys[i], buff), N->ptrs[i+1]); 
		}
		fprintf(out, "/\n");
		for(i=0; i<N->cnt+1; i++) {
			if( N->ptrs[i] == NULL) {
			} else {
					dump_tree_uniq(N->ptrs[i], level+1, kt);
				if( ((bpt_node_t)(N->ptrs[i]))->parent != N) {
					tb_warn("reparenting pb !!\n");
					exit(-1);
				}
			}
		}
	} else {
		fprintf(out, "+-Leaf[%p:%d]::%p : ", N, N->cnt, N->parent);
		for(i=0; i<N->cnt; i++) {
			fprintf(out, "/%s", k2sz(N->keys[i], buff));
		}
		if(N->cnt == 0) abort();
		fprintf(out, "/\n");
	}
	}
}


static void dump_tree_mult(bpt_node_t N, int level, int kt) {
	int i;
	FILE *out = stderr;
	k2sz_t k2sz = kt_getK2sz(kt);
	char buff[100];


	if( level == 0 ) {
		fprintf(out, "-----------------------------\n");
	}
	for(i=0; i<level; i++) {
		fprintf(out, "|");
	}
	if( N->type == BPT_NODE ) {
		fprintf(out, "+ Node[%p:%d]::%p : %p", N, N->cnt, N->parent, N->ptrs[0]);
		for(i=0; i<N->cnt; i++) {
			fprintf(out, " <= %s < %p", k2sz(N->keys[i], buff), N->ptrs[i+1]); 
		}
		fprintf(out, "/\n");
		for(i=0; i<N->cnt+1; i++) {
			if( N->ptrs[i] != NULL) {
				dump_tree_mult(N->ptrs[i], level+1, kt);
				if( ((bpt_node_t)(N->ptrs[i]))->parent != N) {
					tb_warn("reparenting pb !!\n");
					exit(-1);
				}
			}
		}
	} else {
		fprintf(out, "+-Leaf[%p:%d]::%p : ", N, N->cnt, N->parent);
		for(i=0; i<N->cnt; i++) {
			fprintf(stderr, "/<%d>%s", ((bpt_dupes_t)N->ptrs[i])->nb, k2sz(N->keys[i], buff));
		}
		fprintf(out, "/\n");
	}
}


void dump_chain(bptree_t Btree) {
	int i;
	bpt_node_t n = Btree->first;
	k2sz_t k2sz = kt_getK2sz(Btree->kt);
	char buff[100];
	while(n != NULL) {
		fprintf(stderr, "%p:(%d)     : %p <-", n, n->cnt, n->prev);
		for(i = 0; i<n->cnt; i++) {
			if(Btree->dupe_policy == 1) {
				fprintf(stderr, "/<%d>%s", ((bpt_dupes_t)bpt_get_node_ptr(n,i))->nb, 
								k2sz(bpt_get_node_key(n, i), buff));
			} else {
				fprintf(stderr, "/%s", k2sz(bpt_get_node_key(n, i), buff));
			}
		}
		fprintf(stderr, " ->%p\n", n->next);
		n = n->next;
	}

}

int check_chain(bptree_t Btree) {
	int i;
	bpt_node_t n = Btree->first;
	kcmp_t kcmp = kt_getKcmp(Btree->kt);
	char buff[100];
	while(n != NULL) {
		for(i = 0; i<n->cnt-1; i++) {
			if( kcmp(bpt_get_node_key(n, i), bpt_get_node_key(n, i+1)) >=0) {
				tb_warn("chain error in %p: %s ! < %s\n", n,
								kt_getK2sz(Btree->kt)(bpt_get_node_key(n, i), buff),
								kt_getK2sz(Btree->kt)(bpt_get_node_key(n, i+1), buff));
				abort();
			}
		}
		n = n->next;
	}
	return 1;
}

void clear_node(bpt_node_t N, int hasDupes, kfree_t kfree) {
	int i;
	if(N->type == BPT_LEAF) {
		for(i=0; i<N->cnt; i++) {
			if(hasDupes) {
				int d;
				for(d=0; d < ((bpt_dupes_t)N->ptrs[i])->nb; d++) {
					TB_UNDOCK(((bpt_dupes_t)N->ptrs[i])->values[d]);
					tb_Free(((bpt_dupes_t)N->ptrs[i])->values[d]);
				}
				tb_xfree(((bpt_dupes_t)N->ptrs[i])->values);
				tb_xfree(((bpt_dupes_t)N->ptrs[i]));
			} else {
				TB_UNDOCK(N->ptrs[i]);
				tb_Free(N->ptrs[i]);
			}
			kfree(N->keys[i]);
		}
	} else {
		for(i=0; i<N->cnt+1; i++) {
			if(N->ptrs[i]) clear_node(N->ptrs[i], hasDupes, kfree);
			if(i<N->cnt) kfree(N->keys[i]); // N->cnt ptrs but N->cnt-1 keys
		}
	}		
	tb_xfree(N->keys);
	tb_xfree(N->ptrs);
	tb_xfree(N);
}

Dict_t tb_dict_clear(Dict_t D) {
	bptree_t Btree = XBpt(D);
	bpt_node_t node = Btree->root;
	if(Btree->cnt != 0) {
		fm_fastfree_on();
		clear_node(node, Btree->dupe_policy, kt_getKfree(Btree->kt));
		fm_fastfree_off();
		Btree->cnt = Btree->size = 0;
		Btree->root = Btree->first = Btree->last = NULL;
		Btree->root = bpt_newNode( Btree );
	}
	return D;
}

void * tb_dict_free(Dict_t D) {
	bptree_t Btree = XBpt(D);
	tb_dict_clear(D);
	//obsolete	bpt_freeIterCtx(D);

	tb_xfree(Btree);
	tb_freeMembers(D);
	return tb_getParentMethod(D, OM_FREE);
}


int tb_Dict_numKeys(Dict_t D) {
	if( tb_valid(D, TB_DICT, __FUNCTION__)) {
		return XBpt(D)->cnt;
	}
	return -1;
}

			
