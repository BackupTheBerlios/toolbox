//==================================================
// $Id: RW_Lock.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//==================================================
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

#ifndef __RW_LOCK_H
#define __RW_LOCK_H

#define TB_RLOCK 1
#define TB_WLOCK 2

#include "pthread.h"

struct tb_lock {
	pthread_t       tid;
	pthread_mutex_t mtx;
	pthread_cond_t  cond;
	int             access;
};
typedef struct tb_lock *tb_lock_t;


struct share {
	tb_lock_t       lock;
	struct share  * next;
	struct share  * prev;
};
typedef struct share *share_t;

struct share_list {
	share_t first;
	share_t last;
	int     nb;
};
typedef struct share_list *share_list_t;


struct rw_lock {
	share_list_t    sharers;
	share_list_t    lockers;
	int             writer_tid;
	pthread_mutex_t mlock;
};
typedef struct rw_lock *rw_lock_t;



struct lock_manager {
	pthread_mutex_t mtx;
	Hash_t          sharing;
};
typedef struct lock_manager *lock_manager_t;


#endif
