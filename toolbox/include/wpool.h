//==================================================
// $Id: wpool.h,v 1.1 2004/05/12 22:04:49 plg Exp $
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

#ifndef WPOOL_H
#define WPOOL_H
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include "Toolbox.h"


enum job_state {
	JS_ERROR=-1,
	JS_READY=0,
	JS_QUEUED,
	JS_ACCEPTED,
	JS_BUSY,
	JS_COMPLETED
};
typedef enum job_state job_state_t;

struct job {

	pthread_mutex_t    mtx;
	pthread_cond_t     cond;

	worker_cb_t        work;
	void             * data;
	int                mode;
	job_state_t        state;

	struct job      * next;    
	struct job      * prev;    
};
//typedef struct job *job_t;

struct job_list {
	job_t   first;
	job_t   last;
	int     nb;
};
typedef struct job_list *job_list_t;

enum work_state {
	WS_STARTING=0,
	WS_IDLE,
	WS_BUSY,
	WS_STOPPED,
};

enum work_order {
	START_JOB=1,
	STOP,
	WAIT,
};



struct worker {
	int                id;
	int                timeout;
	pthread_t          instance;
	struct WPool     * pool;

	pthread_mutex_t    mtx;
	pthread_cond_t     cond;
	enum work_state    state;
	time_t             state_start;
	enum work_order    request;
	job_t              cur_job;
 
	struct worker    * next;
	struct worker    * prev;
};
typedef struct worker *worker_t;

struct worker_list {
	int        nb;
	worker_t   first;
	worker_t   last;
};
typedef struct worker_list *worker_list_t;


struct WPool {
	int             start_nb;
	int             max_workers;
	int             min_workers;
	int             timeout;

	job_list_t      pending_jobs;
	job_list_t      completed_jobs;
	pthread_mutex_t completed_mtx;
	pthread_cond_t  completed_cond;

	worker_list_t   at_work;
	worker_list_t   at_rest;

	pthread_mutex_t mtx;
	pthread_cond_t  cond;
};
//typedef struct WPool *WPool_t;






#endif
