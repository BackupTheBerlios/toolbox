//------------------------------------------------------------------
// $Id: fastmem.h,v 1.2 2005/05/12 21:54:36 plg Exp $
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

#ifndef FASTMEM_H
#define FASTMEM_H

#include <stdio.h>
#include <pthread.h>
#include "Toolbox.h"

#ifndef TB_FASTMEM_VEC_SIZE
#define TB_FASTMEM_VEC_SIZE   1024*512
#endif


struct chunk {
	unsigned int  status;
		unsigned int  refcnt;
	unsigned int    size;
	struct chunk  * prev;
	struct chunk  * next;
};
typedef struct chunk *chunk_t;


/* struct chunk { */
/* 	unsigned char  status; */
/* 	unsigned char  rfu; */
/* 	unsigned short cnt; */
/* 	//	unsigned int  refcnt; */
/* 	unsigned int    size; */
/* 	struct chunk  * prev; */
/* 	struct chunk  * next; */
/* }; */
/* typedef struct chunk *chunk_t; */


struct tx_debug {
	void   ** cs; // fixme: obsolete !
	int       cs_frames_nb;
	FILE    * fm_trace_fh;
};
typedef struct tx_debug *tx_debug_t;

struct fastMem_vector {
	void *           base;
	int              free_slots;
	int              used_slots;
	int              free;
	int              used;	
	chunk_t          free_chunks;
	int              largest_free;
	int              dirty;
};
typedef struct fastMem_vector *fastMem_vector_t;

struct fastMem {
	pthread_mutex_t lock;

	fastMem_vector_t *vectors;          // big vects of mem semi-permanent
	fastMem_vector_t *huge_chunks;      // blocs bigger than vect size (delt as exceptions)
	int          vectors_nb;            // currently alloc'ted vector nb
	int          vectors_size;          // 
	int          min_chunk_sz;          // lower granularity
	int          max_vectors;           // big max allowed
	int          nb_free_vectors;       // currently unused vectors (1 freed when 2 unused)
	int          enlarge_nb;            // nb of vector adding events
	int          shrink_nb;             // nb of vector freeing events
	int          glob_free;             // stats values
	int          glob_used;
	int          glob_free_slots;
	int          glob_used_slots;
	int          glob_dirty;            // marker for sweeping and tidying vectors
	int          highwatermark;         // biggest use of mem
	int          largest;               // biggest one chunk
	int          free_call;             // calls stats
	int          malloc_call;
	int          realloc_call;
	int          calloc_call;
	int          strdup_call;
	int          fastfree;              // marker for 'fast & dirty' freeing

	tx_debug_t   dbg;
};
typedef struct fastMem *fastMem_t;

#define UI(A)   ((unsigned int)(A))
#define US(A)   ((unsigned short)(A))

#define SZCHUNK sizeof(struct chunk)
#define USED US(0)
#define FREE US(1)
#define FMgetBase(A) (void*)(UI((A))+SZCHUNK)
#define FMgetChunk(A) (chunk_t)(UI((A))-SZCHUNK)

extern fastMem_t fm;

void fm_fastfree_on();
void fm_fastfree_off();

void  * fm_malloc       (size_t sz);
void  * fm_realloc      (void *mem, size_t new_size);
void  * fm_calloc       (size_t sz, size_t nb);
void    fm_free         (void *mem);
char  * fm_strdup       (char *str);
int     fm_addref       (void *mem);
int     fm_unref        (void *mem);
void fm_addObject(tb_Object_t O);
void fm_delObject(tb_Object_t O);
void fm_dropObject(tb_Object_t O);
void fm_recycleObject(tb_Object_t O);

void _setup_fm();
#endif
