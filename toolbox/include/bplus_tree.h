//----------------------------------------------------------------------------------
// $Id: bplus_tree.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//----------------------------------------------------------------------------------
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

#ifndef BPLUS_TREE_H
#define BPLUS_TREE_H

#include "Toolbox.h"
#include "Dict.h"
#include "iterators.h"
#include "Ktypes.h"


enum { 
	BPT_LEAF = 0,
	BPT_NODE 
};

enum op {
	EQ = 1, // ==
	LT,     // <
	GT,     // >
};


struct bptree_node {
	int                   type;
	struct bptree_node  * parent;
	tb_Key_t            * keys;
	void               ** ptrs;
	int                   cnt;
	struct bptree_node  * next;
	struct bptree_node  * prev;
};
typedef struct bptree_node *bpt_leaf_t;
typedef bpt_leaf_t bpt_node_t;

typedef void (*dumptree_t)(bpt_node_t,int);

struct bptree {
	int            size;
	int            cnt;
	int            branch_factor;
	int            dupe_policy;
	int            kt;
	bpt_node_t     root;
	bpt_leaf_t     first;
	bpt_leaf_t     last;
};
typedef struct bptree *bptree_t;

struct bpt_dupes {
	int     nb;
	void ** values;
};
typedef struct bpt_dupes *bpt_dupes_t;

// public
Dict_t       tb_dict_new         (tb_Object_t O, int keytype, int allow_dupes);
void *       tb_dict_free        (Dict_t D);
int          tb_dict_getsize     (Dict_t D);
void         tb_dict_dump        (Dict_t D, int level);
Dict_t       tb_dict_clear       (Dict_t D);
int          tb_dict_insert      (Dict_t Dict, void *value, tb_Key_t key);
int          tb_dict_replace     (Dict_t Dict, void *value, tb_Key_t key);
tb_Object_t  tb_dict_get         (Dict_t D, tb_Key_t key);
tb_Object_t  tb_dict_take        (Dict_t D, tb_Key_t key);
int          tb_dict_remove      (Dict_t D, tb_Key_t key);
int          tb_dict_exists      (Dict_t D, tb_Key_t key);



//bptree_t bpt_newTree    (int branch_factor, bpt_ktype_t ktype);
void    dump_chain      (bptree_t Btree);

//void    dump_tree_uniq  (bpt_node_t node, int level);
//void    dump_tree_mult  (bpt_node_t node, int level);
int     check_chain     (bptree_t Btree);

int bpt_upper_bound   (bptree_t Btree, tb_Key_t key, bpt_node_t *Node, int *Ndx);
int bpt_lower_bound   (bptree_t Btree, tb_Key_t key, bpt_node_t *Node, int *Ndx);
bpt_node_t bpt_lookup_node   (bptree_t Btree, tb_Key_t key);


struct dictIter {
	bptree_t    Btree;
	bpt_leaf_t  cur_node;
	int         cur_ndx;
	int         hasDupes;
	int         dup_ndx;
};
typedef struct dictIter *dictIter_t;
//#define XBPTIT(A)   ((dictIter_t)A->extra)


_iterator_ctx_t bpt_getIterCtx       (Iterator_t D);
_iterator_ctx_t bpt_newIterCtx       (Iterator_t D);
void            bpt_freeIterCtx      (Iterator_t D);
retcode_t       bpt_goFirst          (Iterator_t D);
retcode_t       bpt_goLast           (Iterator_t D);
tb_Object_t     bpt_goNext           (Iterator_t D);
tb_Object_t     bpt_goPrev           (Iterator_t D);
tb_Key_t        bpt_curKey           (Iterator_t D);
tb_Object_t     bpt_curVal           (Iterator_t D);
retcode_t       bpt_find             (Iterator_t It, tb_Key_t key);
retcode_t       bpt_lookup           (bptree_t Btree, tb_Key_t key, 	bpt_node_t *node, int *n);


inline bptree_t XBpt(Dict_t This);

#endif
