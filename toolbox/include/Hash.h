// $Id: Hash.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//====================================================================
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

#ifndef __TB_HASH_H__
#define __TB_HASH_H__

#include "Containers.h"
#include "iterators.h"


#define HASH_MIN_SIZE 11
#define HASH_MAX_SIZE 13845163


struct Node {
  char           * key;
  tb_Object_t      value;
	Varray_t         duplicates;
  struct Node    * next;
  struct Node    * prev;
};

typedef struct Node * Node_t; // fixme: way too common !! must be prefixed
#define PNODES(A)    ((Node_t *)(A)) 

struct hashIter {
	int         inconsistency; // flag to be set when iterator no more in sync w/ container
	Hash_t      H;
	Node_t      first;
	int         first_ndx;
	Node_t      last;
	int         last_ndx;
	Node_t      cur_node;
	int         cur_ndx;
};
typedef struct hashIter *hashIter_t;

struct hash_extra {
  int        size;
	void     * hash_fnc;
	int        hash_fnc_type;
  int        frozen;
	int        flag;
	//obsolete	hashIter_t iter;
};
typedef struct hash_extra *hash_extra_t;



#define TB_H_NORMALIZE_KEYS        1

hashIter_t         hash_getIterCtx       (Hash_t H);
hashIter_t         hash_newIterCtx       (Hash_t H);
void               hash_freeIterCtx      (Hash_t H);
RETCODE            hash_goFirst          (Iterator_t It);
RETCODE            hash_goLast           (Iterator_t It);
tb_Object_t        hash_goNext           (Iterator_t It);
tb_Object_t        hash_goPrev           (Iterator_t It);
tb_Key_t           hash_curKey           (Iterator_t It);
tb_Object_t        hash_curVal           (Iterator_t It);




#define HASH_ME(O,S) ( ((unsigned long (*)(char *,size_t))\
                       ((hash_extra_t)O->extra)->hash_fnc)(S,(strlen(S))))

// sucked from glib

/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 */

static const uint _primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};


#endif

