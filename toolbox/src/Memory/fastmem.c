//====================================================
//$Id: fastmem.c,v 1.3 2004/07/01 21:39:15 plg Exp $
//====================================================
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
 * @file fastmem.c Toolbox memory helpers functions
 */

/**
 * @defgroup Memory Memory manager
 * Self memory allocator/manager
 */

/* TODO:

	 + allow refcounting from userland (badly broken)

*/



#ifdef USE_FASTMEM
#  undef USE_FASTMEM
#endif

#ifndef __BUILD
#  define __BUILD
#endif

#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "fastmem.h"
#include "Toolbox.h"
#include "Objects.h"
#include "Memory.h"

#define p2o(A,B) (((B)>0)? ( UI((B)) - UI((A))) : 0)

pthread_once_t _setup_fm_once = PTHREAD_ONCE_INIT;  

void * (*tb_xmalloc)  (size_t)            = fm_malloc;
void * (*tb_xcalloc)  (size_t, size_t)    = fm_calloc;
void * (*tb_xrealloc) (void *, size_t)    = fm_realloc;
char * (*tb_xstrdup)  (char *)            = fm_strdup;
void   (*tb_xfree)    (void *)            = fm_free;


// big bad glob
fastMem_t fm;
char *fm_tag[] = {"USED", "FREE"};

static void              remove_chunk              (int free, fastMem_vector_t Vec, chunk_t chunk);
static void              insert_free_chunk         (fastMem_vector_t Vec, chunk_t chunk);
static void              insert_used_chunk         (fastMem_vector_t Vec, chunk_t chunk);
static void              clean_free_chunks         (void);
static int               setup_fastmem             (int vec_size, int debug);
static void              fm_enlarge_heap           ();
static void              fm_shrink_heap            (int vec_ndx);
static void              shrink_if_shrinkable      (fastMem_vector_t vec);
static fastMem_vector_t  findVec                   (void *mem);
static int               findVec_ndx               (fastMem_vector_t vec);
static void              __fm_Dump                 ();
static void            * __fm_malloc               (size_t sz);
static void              __fm_free_no_lock         (void *mem);

#define assert_vec_is_clean(Vec) assert(((Vec->free_slots+Vec->used_slots)*SZCHUNK+Vec->free\
 +Vec->used)-fm->vectors_size == 0)


#ifdef TB_MEM_DEBUG
#define TB_FASTMEM_DEBUG
#endif



#ifdef TB_FASTMEM_DEBUG
static void _fm_check(char *, int);
//static void _fm_checkbound(char *, int, chunk_t);

#define fm_check(A,B) _fm_check(A,B)
#define fm_checkbound(A,B,C) /*nothing*/
//#define fm_checkbound(A,B,C) _fm_checkbound(A,B,C)

static void _fm_check(char *where, int line) {
	
	if( (fm->glob_used_slots + fm->glob_free_slots) *SZCHUNK + fm->glob_free + fm->glob_used != 
			fm->vectors_size*fm->vectors_nb) {
		tb_error("fm_check failed in <%s:%d>\n", where, line);
		__fm_Dump();
		abort();
	}
}
/* fixme: this assumes that every chunk is tagged FREE/USED. 
static void _fm_checkbound(char *where, int line, chunk_t chunk) {
	if(chunk->status == FREE || chunk->status ==USED) return;
	tb_error("fm_checkbound failed in <%s:%d> (%d@%p)\n", 
					 where, line, chunk->status, FMgetBase(chunk));
	abort();
}
*/
#else
#define fm_check(A,B) /*nothing*/
#define fm_checkbound(A,B,C) /*nothing*/
#endif



static void setup_fm_once(void) {
	// getenv ...
	// rcfile ...
	int vec_size = TB_FASTMEM_VEC_SIZE;

#ifdef TB_MEM_DEBUG
	int debug_info=1;
#else 
	int debug_info=0;
#endif
	setup_fastmem(vec_size, debug_info);
}

static void fm_enlarge_heap() {
	chunk_t chunk;
	fastMem_vector_t Vec;
	if( fm->vectors_nb < fm->max_vectors) {
		fm->enlarge_nb++;
		fm->vectors_nb++;
		if(( fm->vectors = realloc(fm->vectors, sizeof(fastMem_vector_t) *fm->vectors_nb)) == NULL) {
			abort();
		}
		
		Vec = fm->vectors[fm->vectors_nb -1] = calloc(1, sizeof(struct fastMem_vector));
		if(Vec == NULL) abort();
		if((Vec->base = calloc(1, (size_t)fm->vectors_size)) == NULL) {
			abort();
		}
		chunk = (chunk_t) Vec->base;
		chunk->status = FREE;
		chunk->size = fm->vectors_size - SZCHUNK;
		chunk->next = chunk->prev = NULL;
		Vec->free_chunks = chunk;
		Vec->free_slots = 1;
		Vec->free = chunk->size;
		fm->glob_free_slots++;
		fm->glob_free += chunk->size;
		fm->nb_free_vectors++;

	} else {
		tb_warn("max allocatable mem reached (%d vectors of %d bytes)\n",
						fm->max_vectors, fm->vectors_size);
		pthread_mutex_unlock(&fm->lock);
		__fm_Dump();
		abort();
	}
}

/** Debug function to keep trace of object allocation
 * Activated by TB_MEM_DEBUG macro constant definition
 * @ingroup Memory
 */
void fm_addObject(tb_Object_t O) {
	mdbg_t md = get_tb_mdbg();
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, 
						"+O{%ld}:%p:%s:%s:%s:%d\n",
						pthread_self(), O, tb_nameOf(O->isA), md->func, md->file, md->line);
	}
	pthread_mutex_unlock(&fm->lock);
}
 
/** Debug function to keep trace of object de-allocation
 * Activated by TB_MEM_DEBUG macro constant definition
 * @ingroup Memory
 */
void fm_delObject(tb_Object_t O) {
	mdbg_t md = get_tb_mdbg();
 
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, 
						"-O{%ld}:%p:%s:%s:%s:%d\n",
						pthread_self(), O, tb_nameOf(O->isA), md->func, md->file, md->line);
	}
	pthread_mutex_unlock(&fm->lock);
}


static void fm_shrink_heap(int vec_ndx) {
	fastMem_vector_t Vec = fm->vectors[vec_ndx];
	fm->shrink_nb++;
	memmove((void *) (UI(fm->vectors) + (sizeof(fastMem_vector_t) * vec_ndx)),
					(void *) (UI(fm->vectors) + (sizeof(fastMem_vector_t) * (vec_ndx+1))),
					(sizeof(fastMem_vector_t) * fm->vectors_nb)-(sizeof(fastMem_vector_t) *vec_ndx));
	free(Vec->base);
	free(Vec);
	fm->glob_free -= (fm->vectors_size - SZCHUNK);
	fm->glob_free_slots --;
	fm->vectors_nb--;
	fm->nb_free_vectors--;
}

static int setup_fastmem(int vec_size, int debug_info) {
	char *s;
	int sz = vec_size;
	int max = 100;
	int min = 32;

	fm = calloc(1, sizeof(struct fastMem));
	pthread_mutex_init(	&fm->lock, NULL);


	if((s = getenv("fm_vector_size")))     sz  = TB_MAX(UI( atoi(s)), 1024);
	if((s = getenv("fm_max_vectors")))     max = TB_MAX(UI( atoi(s)), 1);
	if((s = getenv("fm_min_chunk_size")))  min = TB_MAX(UI( atoi(s)), 32);

	fm->min_chunk_sz = min;
	fm->max_vectors  = max;
	fm->vectors_size = sz;
	if((s = getenv("fm_debug"))) {
		fm->dbg = calloc(1, sizeof(struct tx_debug));
		if((fm->dbg->fm_trace_fh = fopen(s, "w"))) {
			setvbuf(fm->dbg->fm_trace_fh, (char *)NULL, _IOLBF, 0);
		}
	}
	return 1;
}

static fastMem_vector_t findVec(void *mem) {
	int i;
	for(i=0; i<fm->vectors_nb; i++) {
		if(UI(fm->vectors[i]->base) < UI(mem) && 
			 UI(mem) < UI(fm->vectors[i]->base)+fm->vectors_size) {
			return fm->vectors[i];
		}
	}
	return NULL;
}

static int findVec_ndx(fastMem_vector_t vec) {
	int i;
	for(i=0; i<fm->vectors_nb; i++) {
		if(fm->vectors[i] == vec) return i;
	}
	return -1;
}

/** Helper function to indicate start of many subsequent deallocation
 * Freeze internal memory chunks cleaners, to speed-up batches of freeing
 * @ingroup Memory
 */
void fm_fastfree_on() {
	if(1) {
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);
	fm->fastfree ++;
	pthread_mutex_unlock(&fm->lock);
	}
}

/** Helper function to indicate end of many subsequent deallocation
 * Freeze internal memory chunks cleaners, to speed-up batches of freeing
 * @ingroup Memory
 */
void fm_fastfree_off() {
	if(1) {
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);

	fm->fastfree--;
	if(fm->fastfree <=0) {
		fm->fastfree = 0;
			fm_check(__FILE__,__LINE__);
		if( fm->glob_dirty ) {
			clean_free_chunks();
			fm->glob_dirty = 0;
		}
	}
	pthread_mutex_unlock(&fm->lock);
	}
}

static void shrink_if_shrinkable(fastMem_vector_t Vec) {
	if(Vec->used_slots == 0) {
		if(fm->nb_free_vectors >1) {
			fm_shrink_heap( findVec_ndx(Vec) );
		} else {
			fm->nb_free_vectors++;
		}
	}	
}

void _setup_fm() {
	pthread_once(&_setup_fm_once, setup_fm_once);
}

/** reimplacement allocator for malloc
 * @ingroup Memory
 */
void *xMalloc(char* func, char *file, int line, size_t sz) {
	void *mem = fm_malloc(sz);
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "+M{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, sz, func, file, line);
	}
	pthread_mutex_unlock(&fm->lock);
	return mem;
}

void *fm_malloc(size_t sz) {
	void *mem;
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);
	mem  = __fm_malloc(sz);
	pthread_mutex_unlock(&fm->lock);
	return mem;
}

static void *__fm_malloc(size_t sz) {
	chunk_t chunk;
	int i, rsz;

	//	pthread_once(&_setup_fm_once, setup_fm_once);

	rsz = TB_MAX(fm->min_chunk_sz, sz);

	if( rsz + SZCHUNK >= fm->vectors_size - SZCHUNK) {
		// ->default to libc mallocator
		return malloc(sz);
	}
	fm->malloc_call ++;

	if(fm->fastfree) {
		clean_free_chunks();
	}

	if( fm->glob_free < rsz + SZCHUNK ) {
		fm_enlarge_heap();
	}

 retry:

	for( i=0; i<fm->vectors_nb; i++) {
		fastMem_vector_t Vec = fm->vectors[i];
		if( Vec->free <  rsz + SZCHUNK ) continue;

		chunk = Vec->free_chunks;

		if( chunk == NULL) {
			tb_warn("no more free chunks in Vec[%d]!! (%d bytes needed (%d))\n", i, sz, rsz+SZCHUNK);

			__fm_Dump();
			abort();
		}

		do {

			if( chunk->size > (rsz + SZCHUNK) ) {

				if( chunk->size - (rsz + SZCHUNK) > (fm->min_chunk_sz + SZCHUNK)) {

					// cut a new used chunk into this big free chunk
					chunk_t C = (chunk_t)(FMgetBase(chunk) + rsz);
					C->prev = C->next = NULL;
					C->size = chunk->size - (rsz+SZCHUNK);
					// replace chunk by C in free_chunks

					remove_chunk(FREE, Vec, chunk);
					chunk->size = rsz;
					insert_used_chunk(Vec, chunk);
					insert_free_chunk(Vec, C);
			
					fm_check(__FUNCTION__, __LINE__);

					return FMgetBase(chunk);

				} else { // remaining is too small for cutting a another chunk after this one, takin all
					remove_chunk(FREE, Vec, chunk);
					insert_used_chunk(Vec, chunk);
					fm_check(__FUNCTION__, __LINE__);

					return FMgetBase(chunk);
				}
			} else if(chunk->size == rsz) {
				remove_chunk(FREE, Vec, chunk);
				insert_used_chunk(Vec, chunk);

				fm_check(__FUNCTION__, __LINE__);

				return FMgetBase(chunk);
			} 
			chunk = chunk->next;
		} while( chunk != NULL );
	}
	fm_enlarge_heap();

	goto retry;
	
	// not likely to be reached ...

	__fm_Dump();
	abort();
	// even less likely (but avoid gcc warns)
	return NULL;
}


/** reimplacement allocator for strdup
 * @ingroup Memory
 */
char *xStrdup(char* func, char *file, int line, char *str) {
	char *dup = fm_strdup(str);
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh && dup) {
		fprintf(fm->dbg->fm_trace_fh, "+S{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), dup, strlen(dup), func, file, line);
	}
	pthread_mutex_unlock(&fm->lock);
	return dup;
}

char *fm_strdup(char *str) {
	int len;
	void *mem;
	pthread_once(&_setup_fm_once, setup_fm_once);

	if(!str) return NULL;
	pthread_mutex_lock(&fm->lock);

	len = strlen(str);
	mem = __fm_malloc(len+1);
	memcpy(mem, str, len+1);
	fm->strdup_call++;
	pthread_mutex_unlock(&fm->lock);
	return mem;
}

/** reimplacement allocator for realloc
 * @ingroup Memory
 */
void *xRealloc(char* func, char *file, int line, void *mem, size_t sz) {
	void *new = fm_realloc(mem, sz);
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "+R{%ld}:%p:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, new, sz, func, file, line);
	}
	pthread_mutex_unlock(&fm->lock);
	return new;
}

void *fm_realloc(void *mem, size_t new_size) {
	chunk_t chunk, C, G;
	fastMem_vector_t Vec;
	pthread_once(&_setup_fm_once, setup_fm_once);

	if(mem == NULL) return fm_malloc(new_size);

	pthread_mutex_lock(&fm->lock);


	if( (Vec = findVec(mem)) == NULL) {
		// should be vanilla mem (either big chunk or externally allocated)
		tb_info("fm_realloc: vec not found, use vanilla realloc\n");
		pthread_mutex_unlock(&fm->lock);
		return realloc(mem, new_size);
	}

	fm->realloc_call++;

	chunk = (chunk_t)(UI(mem) - SZCHUNK);
	fm_checkbound(__FILE__, __LINE__, chunk);

	if( new_size <= 0) {
		__fm_free_no_lock(chunk);
		return NULL;
	}

	if( chunk->size == new_size) {
		pthread_mutex_unlock(&fm->lock);
		return mem;
	}
 	if( chunk->size < new_size) {
		if( new_size > fm->vectors_size ) {
			// use vanilla allocator
			mem = malloc(new_size);
			memcpy(mem, FMgetBase(chunk), new_size);
			remove_chunk(USED, Vec, chunk);
			insert_free_chunk(Vec, chunk);
			pthread_mutex_unlock(&fm->lock);

			return mem;
		}


		// test if growable without moving
		C = (chunk_t)(FMgetBase(chunk) + chunk->size);

		if(UI(C) < UI(Vec)+fm->vectors_size &&
			 C->status == FREE && C->size > (new_size - chunk->size)) {

			if( C->size > ((new_size - chunk->size) + fm->min_chunk_sz + SZCHUNK)) {
				// C is big enough to make a new free chunk with remaining
				int r;
				G = (chunk_t)(FMgetBase(chunk) + new_size);
				r = C->size - (new_size - chunk->size);
				fm->glob_used += (new_size -chunk->size);
				chunk->size = new_size;
				remove_chunk(FREE, Vec, C);
				G->prev = G->next = NULL;
				G->size = r;
				insert_free_chunk(Vec, G);
				fm_check(__FUNCTION__, __LINE__);
				pthread_mutex_unlock(&fm->lock);

				return FMgetBase(chunk);

			} else { 
				// C is too small to cut a new free chunk: all is given to extend chunk
				chunk->size += C->size +SZCHUNK;
				fm->glob_used += C->size +SZCHUNK;
				remove_chunk(FREE, Vec, C);
				fm_check(__FUNCTION__, __LINE__);
				pthread_mutex_unlock(&fm->lock);

				return FMgetBase(chunk);
			}
		} else {
			void *new;
			// must alloc a new bloc and move previous content
			new = __fm_malloc(new_size);
			memcpy(new, FMgetBase(chunk), chunk->size);
			remove_chunk(USED, Vec, chunk);
			insert_free_chunk(Vec, chunk);

			shrink_if_shrinkable(Vec);
			fm_check(__FUNCTION__, __LINE__);
			pthread_mutex_unlock(&fm->lock);

			return new;
		}

	} else { // new_size < size

		if( chunk->size - new_size > (SZCHUNK+fm->min_chunk_sz) ) {
			int newsize = TB_MAX(fm->min_chunk_sz, new_size);
			if( newsize < chunk->size) {
				C = (chunk_t)(FMgetBase(chunk) + newsize);
				C->size = chunk->size - newsize - SZCHUNK;
				fm->glob_used -= C->size + SZCHUNK;
				Vec->used -= C->size + SZCHUNK;
				insert_free_chunk(Vec, C);
				chunk->size = newsize;

			} 
		} 
	}
	fm_check(__FUNCTION__, __LINE__);
	pthread_mutex_unlock(&fm->lock);


	return mem;
}

/** reimplacement allocator for calloc
 * @ingroup Memory
 */
void *xCalloc(char* func, char *file, int line, size_t nb, size_t sz) {
	void *mem = fm_calloc(sz, nb);
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "+C{%ld}:%p:%d:%s:%s:%d\n", 
						pthread_self(), mem, sz*nb, func, file, line);
	}
	pthread_mutex_unlock(&fm->lock);
	return mem;
}

void *fm_calloc(size_t sz, size_t nb) {	
	void *mem;
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);
	mem = __fm_malloc(sz*nb);
	memset(mem, 0, sz*nb);
	fm->calloc_call++;
	pthread_mutex_unlock(&fm->lock);
	return mem;
}


/** reimplacement deallocator for free
 * @ingroup Memory
 */
void xFree(char* func, char *file, int line, void *mem) {
	fm_free(mem);
	pthread_mutex_lock(&fm->lock);
	if(fm->dbg && fm->dbg->fm_trace_fh) {
		fprintf(fm->dbg->fm_trace_fh, "+F{%ld}:%p:%s:%s:%d\n", 
						pthread_self(), mem, func, file, line);
	}
	pthread_mutex_unlock(&fm->lock);
	return;
}

void fm_free(void *mem) {
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);

	__fm_free_no_lock(mem);

/* 	if((Vec = findVec(mem)) != NULL) { */

/* 		chunk_t chunk = FMgetChunk(mem); */
/* 		fm_checkbound(__FILE__, __LINE__, chunk); */
/* 		fm->free_call ++; */

/* 		remove_chunk(USED, Vec, chunk); */
/* 		insert_free_chunk(Vec, chunk); */

/* 		shrink_if_shrinkable(Vec); */

/* 	} else { */
/* 		// else should be libc's allocage (or dies atrocely) */
/* 		free(mem); */
/* 	} */

	fm_check(__FUNCTION__, __LINE__);
	pthread_mutex_unlock(&fm->lock);
}

void __fm_free_no_lock(void *mem) {
	fastMem_vector_t Vec;

	if((Vec = findVec(mem)) != NULL) {
		chunk_t chunk = FMgetChunk(mem);
		fm_checkbound(__FILE__, __LINE__, chunk);
		fm->free_call ++;

		remove_chunk(USED, Vec, chunk);
		insert_free_chunk(Vec, chunk);

		shrink_if_shrinkable(Vec);

	} else {
		// else should be libc's allocage (or dies atrocely)
		free(mem);
	}
}

				
static void insert_used_chunk(fastMem_vector_t Vec, chunk_t chunk) {
	chunk->status = USED;

	Vec->used_slots ++;
	fm->glob_used_slots ++;
	Vec->used += chunk->size;
	fm->glob_used += chunk->size;
	fm->highwatermark = TB_MAX( fm->highwatermark, fm->glob_used);
	fm->largest = TB_MAX( fm->largest, chunk->size);
	if(Vec->used_slots == 1) fm->nb_free_vectors--;
}

static void remove_chunk(int free, fastMem_vector_t Vec, chunk_t chunk) {
	if(free) {
		fm->glob_free_slots--;
		fm->glob_free -= chunk->size;
		Vec->free_slots--;
		Vec->free -= chunk->size;

		if(chunk->prev == NULL && chunk->next == NULL && Vec->free_slots != 0) {
			tb_error("remove untied chunk %d from Vec %p\n", p2o(Vec->base,chunk), Vec);
			abort();
		}

		if(chunk->prev == NULL) {
			Vec->free_chunks = chunk->next;
		} else {
			chunk->prev->next = chunk->next;
		}
		if(chunk->next) {
			chunk->next->prev = chunk->prev;
		}
	} else {
		fm->glob_used_slots--;
		fm->glob_used -= chunk->size;
		Vec->used_slots--;
		Vec->used -= chunk->size;

	}

}


static void insert_free_chunk(fastMem_vector_t Vec, chunk_t chunk) {
	chunk_t fc = Vec->free_chunks;
	chunk_t prev = NULL;
	chunk->status = FREE;
	fm->glob_free += chunk->size;
	fm->glob_free_slots ++;
	Vec->free += chunk->size;
	Vec->free_slots ++;

	if(fm->fastfree) {
		fm->glob_dirty++;
		Vec->dirty++;
		return;
	}
	
	if(fc == NULL) {
		Vec->free_chunks = chunk;
		chunk->prev = chunk->next = NULL;
		return;
	}


	do {
		if(prev) {
			if( ((void *) (UI(FMgetBase(prev)) + prev->size)) == chunk) { // merge with prev free
				fm->glob_free_slots --;
				fm->glob_free += SZCHUNK;
				Vec->free_slots --;
				Vec->free += SZCHUNK;
				prev->size += chunk->size + SZCHUNK;
				chunk = prev;

				if(((void *) (UI(FMgetBase(chunk)) + chunk->size)) == fc) { // merge with next free
					fm->glob_free_slots --;
					fm->glob_free += SZCHUNK;
					Vec->free_slots --;
					Vec->free += SZCHUNK;
					chunk->next = fc->next;
					if(chunk->next) chunk->next->prev = chunk;
					chunk->size += fc->size + SZCHUNK;
				}
				return;
			}
		}

		if( UI(FMgetBase(fc)) > UI(FMgetBase(chunk))) {
			if(((void *) (UI(FMgetBase(chunk)) + chunk->size)) == fc) { // merge with next free
				fm->glob_free_slots --;
				fm->glob_free += SZCHUNK;
				Vec->free_slots --;
				Vec->free += SZCHUNK;
				if( Vec->free_chunks == fc ) {
					Vec->free_chunks = chunk;
					chunk->size += fc->size + SZCHUNK;
					chunk->prev = NULL;
					chunk->next = fc->next;
					if(chunk->next) chunk->next->prev = chunk;
				} else {
					chunk->prev = fc->prev;
					if(chunk->prev) chunk->prev->next = chunk;
					chunk->next = fc->next;
					if(chunk->next) chunk->next->prev = chunk;
					chunk->size += fc->size + SZCHUNK;
				}
			} else {
				if( Vec->free_chunks == fc ) {
					Vec->free_chunks = chunk;
					chunk->prev = NULL;
					chunk->next = fc;
					fc->prev= chunk;
				} else {
					if( (chunk->prev = fc->prev)) chunk->prev->next = chunk;
					fc->prev  = chunk;
					chunk->next = fc;
				}
			}
			return;
		}
		prev = fc;
		fc = fc->next;
	} while(fc); 

	prev->next = chunk;
	chunk->prev = prev;
	chunk->next = NULL;
}



/*
	relink free chunks, merging continuous chunks
 */
static void clean_free_chunks() {

	int i;

	pthread_once(&_setup_fm_once, setup_fm_once);

	for(i=0; i<fm->vectors_nb; i++) {
		fastMem_vector_t Vec = fm->vectors[i];
		chunk_t prev = NULL, prev_free = NULL, fc = (chunk_t)Vec->base;

		if(Vec->dirty == 0) continue;

#ifdef TB_FASTMEM_DEBUG
		tb_notice("CleanVector[%d]@%p /sz=%d base=%p/Fslots:%d(%d)/Uslots:%d(%d)/dirty=%d\n", 
							i, Vec, fm->vectors_size, Vec->base, Vec->free_slots, Vec->free,
							Vec->used_slots, Vec->used,Vec->dirty);
#endif

		Vec->free_chunks = NULL;
		Vec->free_slots = 0;
		Vec->free = 0;
		Vec->used_slots = 0;
		Vec->used = 0;
			

		while( UI(fc) <  UI(Vec->base)+fm->vectors_size) {
			if(fc->status == FREE) {
				if(!prev) {
					prev = fc;

					if(Vec->free_chunks == NULL) {				
						Vec->free_chunks = fc;
						prev_free = fc;
						fc->prev = NULL;
						fc->next = NULL;
					} else {
						prev_free->next = fc;
						fc->prev = prev_free;
						fc->next = NULL;
						prev_free = fc;
					}
					Vec->free_slots ++;
					Vec->free += fc->size;

				} else {
					prev->size += fc->size + SZCHUNK;
					fm->glob_free += SZCHUNK;
					fm->glob_free_slots --;
					Vec->free += fc->size + SZCHUNK;
				}
			} else { // fc->status == USED
				prev = NULL;

				Vec->used_slots ++;
				Vec->used += fc->size;
			}
			fc = (chunk_t) (UI(fc)+SZCHUNK+fc->size);
		}
		Vec->dirty = 0;
#ifdef TB_FASTMEM_DEBUG
		tb_notice("CleanedVector[%d]@%p /sz=%d base=%p/Fslots:%d(%d)/Uslots:%d(%d)/dirty=%d [%d]\n", 
							i, Vec, fm->vectors_size, Vec->base, Vec->free_slots, Vec->free,
							Vec->used_slots, Vec->used,Vec->dirty,
							((Vec->free_slots+Vec->used_slots) *SZCHUNK + Vec->free +Vec->used)-fm->vectors_size);
#endif
	

		if(Vec->used_slots == 0) {
			fm->nb_free_vectors++;
			if(fm->nb_free_vectors >1) {
				fm_shrink_heap(i);
				i--;
			}
		}
	}
} 




/** Debug function to list every memory chunks
 * @ingroup Memory
 */
void fm_dumpChunks() {
	chunk_t chunk;
	int fsz = 0;
	int i;
	pthread_once(&_setup_fm_once, setup_fm_once);
	//	pthread_mutex_lock(&fm->lock);

	for(i=0; i<fm->vectors_nb; i++) {
		fastMem_vector_t Vec = fm->vectors[i];
		void *B = Vec->base;
		chunk = Vec->base;
		fprintf(stderr, "Vector[%d]@%p /sz=%d base=%p/Fslots:%d(%d)/Uslots:%d(%d)/dirty=%d\n", 
						i, Vec, fm->vectors_size, Vec->base, Vec->free_slots, Vec->free,
						Vec->used_slots, Vec->used,Vec->dirty);
		while(UI(chunk) < UI(B)+fm->vectors_size) {
			void *p=(void *)(UI(FMgetBase(chunk))+chunk->size);
			if(chunk->status == FREE) {
				fprintf(stderr, "[%d /sz=%d/%s/base=%d-%d (+%d)/  %d <<= =>> %d ]\n", 
								p2o(B,chunk), chunk->size, 
								(chunk->status == FREE) ? "F" : "U",
								(signed int)p2o(B, FMgetBase(chunk)), 
								(signed int)p2o(B, p),
								(chunk->status == FREE) ? (UI(chunk->next) - UI(p)) : 0,
								(chunk->status == FREE) ? p2o(B, chunk->prev) : 0,
								(chunk->status == FREE) ? p2o(B, chunk->next) : 0);
			}
			fsz += chunk->size;
			chunk=(chunk_t)(FMgetBase(chunk) + chunk->size);
		}
	}
	fprintf(stderr, "full size: %d\n", fsz);
}

static void __fm_Dump() {
	int n;
	pthread_once(&_setup_fm_once, setup_fm_once);

	fprintf(stderr, "chunk size=%d\n", SZCHUNK);
	fprintf(stderr, "vectors_nb=%d\n", fm->vectors_nb);
	fprintf(stderr, "enlarge_event=%d\n", fm->enlarge_nb);
	fprintf(stderr, "shrink_event=%d\n", fm->shrink_nb);
	fprintf(stderr, "vectors_size=%d\n", fm->vectors_size);
	fprintf(stderr, "free_slots=%d\n", fm->glob_free_slots);
	fprintf(stderr, "used_slots=%d\n", fm->glob_used_slots);
	fprintf(stderr, "full control blocs size : %d\n", 
					(fm->glob_free_slots+fm->glob_used_slots)*SZCHUNK);
	fprintf(stderr, "free=%d\n", fm->glob_free);
	fprintf(stderr, "used=%d\n", fm->glob_used);
	n = fm->glob_used+ fm->glob_free+ (SZCHUNK* (fm->glob_free_slots+fm->glob_used_slots));
	fprintf(stderr, "used+free+control blocs : %d (%d)\n", n, 
					(fm->vectors_size*fm->vectors_nb) -n);
	fprintf(stderr, "highwatermark=%d\n", fm->highwatermark);
	fprintf(stderr, "largest=%d\n", fm->largest);
	fprintf(stderr, "free_call=%d\n", fm->free_call);
	fprintf(stderr, "malloc_call=%d\n", fm->malloc_call);
	fprintf(stderr, "calloc_call=%d\n", fm->calloc_call);
	fprintf(stderr, "realloc_call=%d\n", fm->realloc_call);
	fprintf(stderr, "strdup_call=%d\n", fm->strdup_call);

	for(n=0; n<fm->vectors_nb; n++) {
		fastMem_vector_t Vec = fm->vectors[n];
		fprintf(stderr, "Vector[%d]@%p /sz=%d base=%p/Fslots:%d(%d)/Uslots:%d(%d)/lf:%d/dirty=%d [%d]\n", 
						n, Vec, fm->vectors_size, Vec->base, 
						Vec->free_slots, Vec->free,
						Vec->used_slots, Vec->used,Vec->largest_free, Vec->dirty,
						((Vec->free_slots+Vec->used_slots) *SZCHUNK + Vec->free +Vec->used)-fm->vectors_size);
	}

}


/** Debug function to display Toolbox memory manager usage
 * @ingroup Memory
 */
void fm_Dump() {
	pthread_once(&_setup_fm_once, setup_fm_once);
	pthread_mutex_lock(&fm->lock);
	__fm_Dump();
	pthread_mutex_unlock(&fm->lock);
}



