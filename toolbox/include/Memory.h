//================================================================
// $Id: Memory.h,v 1.3 2005/05/12 21:54:36 plg Exp $
//================================================================
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

#ifndef __HAVE_TB_MEMORY_H
#define __HAVE_TB_MEMORY_H

#include "fastmem.h"

struct mdbg {
	char *func;
	char *file;
	int line;
};
typedef struct mdbg *mdbg_t;
void set_tb_mdbg(char *func, char *file, int line);
mdbg_t get_tb_mdbg();

char *fm_status(char *buffer, int len);

extern int tb_memtrace;
extern int tb_memdebug;

extern void *(*x_malloc)  (size_t);
extern void *(*x_calloc)  (size_t, size_t);
extern void *(*x_realloc) (void *, size_t);
extern void  (*x_free)    (void *);

struct mList_t {
	uint key;
	int size;
	struct mList_t *next;
	struct mList_t *prev;
};

typedef struct mList_t * mList_t;

void dbg_del_object(tb_Object_t O);
void dbg_add_object(tb_Object_t O);
void dbg_drop_object(tb_Object_t O);
void dbg_recycle_object(tb_Object_t O);
#endif
