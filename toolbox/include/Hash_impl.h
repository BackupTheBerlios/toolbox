/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
//======================================================
// $Id: Hash_impl.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//======================================================

/* Copyright (c) 1999-2002, Paul L. Gatille. All rights reserved.
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

#ifndef __TB_HASH_IMPL_H__
#define __TB_HASH_IMPL_H__

#include "Containers.h"
#include "iterators.h"
#include "tb_ClassBuilder.h"
#include "Iterable_interface.h"


#define HASH_MIN_SIZE 11
#define HASH_MAX_SIZE 13845163


struct tb_hash_node {
  tb_Key_t               key;
	int                    nb;  // duplicates number (0==no duplicates)
  tb_Object_t            value;
	tb_Object_t          * values; // allocated only where H allow duplicates
  struct tb_hash_node  * next;
  struct tb_hash_node  * prev;
};

typedef struct tb_hash_node * tb_hash_node_t;
//#define PNODES(A)    ((tb_hash_node_t *)(A)) 

struct hashIter {
	Hash_t              H;
	tb_hash_node_t      first;
	int                 first_ndx;
	tb_hash_node_t      last;
	int                 last_ndx;
	tb_hash_node_t      cur_node;
	int                 cur_ndx;
	int                 hasDupes;
	int                 dup_ndx;
};
typedef struct hashIter *hashIter_t;

struct hash_extra {
	int               kt;
	int               allow_duplicates;
  int               size;
  int               buckets;
	void            * hash_fnc;
	int               hash_fnc_type;
  int               frozen;
	int               flag;
	tb_hash_node_t  * nodes;

};
typedef struct hash_extra *hash_extra_t;


inline hash_extra_t XHASH(Hash_t H);
inline tb_hash_node_t *XHNODES(Hash_t H);

// initializer for iterable iterface
void register_Hash_Iterable_once(int Object_id);
tb_hash_node_t *tb_hash_lookup_node(Hash_t H, tb_Key_t the_key);
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

