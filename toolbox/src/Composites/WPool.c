//==================================================
// $Id: WPool.c,v 1.1 2004/05/12 22:04:50 plg Exp $
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

#include <stdio.h>
#include "Toolbox.h"
#include "wpool.h"

static int        pool_manage        (WPool_t WP);
static worker_t   newWorker          (int timeout, WPool_t parent_pool);
static int        addWorker          (worker_list_t WL, worker_t W);
static int        addJob             (job_list_t JL, job_t J);
static worker_t   getWorker          (worker_list_t WL, worker_t w);
static void       setWorkerState     (worker_t W, enum work_state ws);
static job_t      getJob             (job_list_t JL);

#define YOU_ARE_FIRED 1
#define I_NEED_YOU    2
#define BACK_TO_WORK  3

static void setWorkerState(worker_t W, enum work_state ws) {
	W->state = ws;
	W->state_start = time(NULL);
}

static void dump_lists(WPool_t WP) {
	worker_t W;
	job_t J;
	fprintf(stderr, "Busy");
	for(W=WP->at_work->first; W; W=W->next) {
		fprintf(stderr, "/id=%d", W->id);
	}
	fprintf(stderr, "/\nRest");
	for(W=WP->at_rest->first; W; W=W->next) {
		fprintf(stderr, "/id=%d", W->id);
	}
	fprintf(stderr, "/\n");
	fprintf(stderr, "Jobs=%d", WP->pending_jobs->nb);
	for(J=WP->pending_jobs->first; J; J=J->next) {
		fprintf(stderr, "/id=%p", J);
	}
	fprintf(stderr, "/\n");

}

static worker_t getWorker(worker_list_t WL, worker_t w) {
	worker_t W = NULL;

	if( WL->nb >0) {
		if(w == NULL) { // take any
			W = WL->last;
			WL->last = W->prev;
			
			if(WL->last) WL->last->next = NULL;
			W->prev = W->next = NULL;
			WL->nb--;
			return W;
		} else { // search w
			for(W = WL->last; W; W = W->prev) {
				if(W->id == w->id) {
					if(W->prev) W->prev->next = W->next;
					if(W->next) W->next->prev = W->prev;
					if(WL->first == W) WL->first = W->next;
					if(WL->last == W) WL->last = W->prev;
					W->next = W->prev = NULL;
					WL->nb--;
					return W;
				}
			}
		}
	}
	tb_warn("can't find worker %d", w->id);
	return W;
}


static job_t getJob(job_list_t JL) {
	job_t J = NULL;
	J = JL->first;
	if( J ) {
		JL->first = J->next;
		if(J->next) J->next->prev = NULL;
		J->prev = J->next = NULL;
		JL->nb--;
		if(JL->nb == 0) JL->first = JL->last = NULL;
	}
	return J;
}



void work_loop(void *data) {
	worker_t W = (worker_t)data;
	pthread_mutex_lock(&W->mtx);
	W->id = pthread_self();
	pthread_cond_signal(&W->cond); // still under lock !
	tb_notice("[worker %d] 'ready to work !'\n", pthread_self());
	while(1) { // always under lock

		if( W->timeout ) {
			struct timeval now;
			struct timespec timeout;
			int rc;
			gettimeofday(&now, NULL);
			timeout.tv_sec = now.tv_sec + W->timeout;
			timeout.tv_nsec = now.tv_usec * 1000;

			rc = pthread_cond_timedwait(&W->cond, &W->mtx, &timeout);

			if(rc == ETIMEDOUT) {
				WPool_t WP = W->pool;
				tb_notice("[worker %d] timeout expired, should I stop ?\n", pthread_self());
				pthread_mutex_lock(&WP->mtx);
				switch( pool_manage(W->pool)) {
				case YOU_ARE_FIRED: 
					tb_warn("got fired !\n");
					pthread_mutex_unlock(&WP->mtx);
					pthread_mutex_unlock(&W->mtx);
					tb_xfree(W);
					pthread_exit(0);
					break;
				case I_NEED_YOU:
					tb_warn("I'll stay a little !\n");
					pthread_mutex_unlock(&W->mtx);
					break;
				}
				continue;
			}			
			tb_notice("[worker %d] got a signal\n", pthread_self());
		} else {
			pthread_cond_wait(&W->cond, &W->mtx);
			tb_notice("[worker %d] got a signal\n", pthread_self());
		}

		do {
			switch(W->request) {
			case WAIT: 
				setWorkerState(W, WS_IDLE);
				pthread_mutex_unlock(&W->mtx);
				break;
			case STOP: 
				{
					WPool_t WP = W->pool;
					pthread_mutex_lock(&WP->mtx);
					tb_notice("[worker %d] stopped by request\n", pthread_self());

					getWorker(WP->at_rest, W);

					pthread_mutex_unlock(&WP->mtx);
					pthread_mutex_unlock(&W->mtx);
					tb_xfree(W);
					pthread_exit(0);
				}
			
				break;
			case  START_JOB:
				{
					WPool_t WP = W->pool;
					tb_notice("[worker %d] new job accepted\n", pthread_self());

					pthread_mutex_lock(&W->cur_job->mtx);
					W->cur_job->state      = JS_BUSY;
					pthread_mutex_unlock(&W->cur_job->mtx);
				
					W->cur_job->work(W->cur_job->data); // real work take place here! 

					tb_notice("[worker %d] job completed\n", pthread_self());

					if(W->cur_job->mode == JOINABLE) {
						pthread_mutex_lock(&W->cur_job->mtx);
						// mv job to completed and send signal
						W->cur_job->state = JS_COMPLETED;
						pthread_mutex_lock(&WP->mtx);
						addJob(WP->completed_jobs, W->cur_job);
						pthread_mutex_unlock(&W->cur_job->mtx);
						pthread_mutex_unlock(&WP->mtx);
						pthread_mutex_lock(&WP->completed_mtx);
						pthread_cond_broadcast(&WP->completed_cond);
						pthread_mutex_unlock(&WP->completed_mtx);
						W->cur_job = NULL;
					} else {
						tb_xfree(W->cur_job);
					}

					pthread_mutex_lock(&WP->mtx);

					// mv W from busy to rest
					addWorker(WP->at_rest, getWorker(WP->at_work, W));
					setWorkerState(W, WS_IDLE);

					switch( pool_manage(W->pool)) {
					case YOU_ARE_FIRED: 
						tb_warn("I got fired (%p)!\n", W);
						W = getWorker(WP->at_rest, W);
						pthread_mutex_unlock(&WP->mtx);
						pthread_mutex_unlock(&W->mtx);
						tb_xfree(W);
						pthread_exit(0);
						break;
					case I_NEED_YOU:
						tb_warn("I'll stay a little !\n");
						break;
					case BACK_TO_WORK:

						W->request = START_JOB;
						W->cur_job = getJob(WP->pending_jobs);
						tb_warn("still something do be done (dequeuing job %p)!\n", W->cur_job);						
						addWorker(WP->at_work, getWorker(WP->at_rest, W));
						setWorkerState(W, WS_BUSY);
						break;
					}
					pthread_mutex_unlock(&WP->mtx);
				}
				break;			
			}
		} while (W->state != WS_IDLE);
	}
}			


static int pool_manage(WPool_t WP) {
	int rc = I_NEED_YOU;
	int cur_w;

	// check pool parameters

	dump_lists(WP);

	cur_w = WP->at_work->nb + WP->at_rest->nb;
	tb_warn("pool_manage: %d busy/ %d at rest (min=%d, max=%d)\n", 
					WP->at_work->nb, WP->at_rest->nb,
					WP->min_workers,WP->max_workers);

	if( cur_w < WP->min_workers ) {
		int i;
		for(i=0; i<(WP->min_workers - cur_w); i++) {
			addWorker(WP->at_rest, newWorker(WP->timeout, WP));
		}
	} else {
		if( WP->pending_jobs->nb >0) {
			rc = BACK_TO_WORK;

		} else if( cur_w > WP->min_workers ) {
			rc = YOU_ARE_FIRED;
		}
	}
	return rc;
}



static worker_t newWorker(int timeout, WPool_t parent_pool) {
	worker_t W = tb_xcalloc(1, sizeof(struct worker));
	pthread_mutex_init(&W->mtx, NULL);
	pthread_cond_init(&W->cond, NULL);

	W->pool        = parent_pool;
	W->timeout     = timeout;
	W->state       = WS_IDLE;
	W->state_start = time(NULL);
	W->request     = WAIT;
	pthread_mutex_lock(&W->mtx);
	pthread_create(&W->instance, NULL, (void *(*)(void*))work_loop, W);
	pthread_cond_wait(&W->cond, &W->mtx);
	pthread_mutex_unlock(&W->mtx);
	return W;
}


job_t newJob(worker_cb_t work, void *data, job_mode_t mode) {
	job_t J   = tb_xcalloc(1, sizeof(struct job));
	J->work   = work;
	J->data   = data;
	J->mode   = mode;
	J->state  = JS_READY;
	return J;
}

	

static int addWorker(worker_list_t WL, worker_t W) {
	if(WL->last != NULL) {
		WL->last->next = W;
		W->next = NULL;
		W->prev = WL->last;
		WL->last = W;
	} else {
		WL->first = WL->last = W;
		W->prev=W->next = NULL;
	}
	return WL->nb++;
}

static int addJob(job_list_t JL, job_t J) {
	job_t j;
	if(JL->last != NULL) {
		tb_warn("addJob in tail");
		JL->last->next = J;
		J->next        = NULL;
		J->prev        = JL->last;
		JL->last       = J;
	} else {
		tb_warn("addJob: first job");
		JL->first = JL->last = J;
		J->prev=J->next = NULL;
	}
	JL->nb++;
	fprintf(stderr, "Jobs=%d", JL->nb);
	for(j=JL->first; j; j=j->next) {
		fprintf(stderr, "/id=%p", j);
	}
	fprintf(stderr, "/\n");

	return JL->nb;
}




WPool_t tb_WPool(int start, int max, int min, int timeout) {
	WPool_t WP = tb_xcalloc(1, sizeof(struct WPool));
	int i;
	pthread_mutex_t mtx;

	WP->start_nb      = start;
	WP->max_workers   = max;
	WP->min_workers   = min;
	WP->timeout       = timeout;
	WP->pending_jobs  = tb_xcalloc(1, sizeof(struct job_list));
	WP->at_work       = tb_xcalloc(1, sizeof(struct worker_list));
	WP->at_rest       = tb_xcalloc(1, sizeof(struct worker_list));

	tb_warn("initialized WP core\n");

	pthread_mutex_init(&WP->mtx, NULL);
	pthread_mutex_lock(&WP->mtx);
	pthread_cond_init(&WP->cond, NULL);
	pthread_mutex_init(&mtx, NULL);


	for(i=0; i<start; i++) {
		tb_warn("initialized WP start workers\n");
		addWorker(WP->at_rest, newWorker(timeout, WP));
	}
	pthread_mutex_unlock(&WP->mtx);
	return WP;
}

	

int tb_submitJob(WPool_t WP, job_t Job) {
	int rc;

	pthread_mutex_lock(&Job->mtx);

	if( Job->state != JS_READY ) {
		tb_warn("invalid job state\n");
		pthread_mutex_unlock(&Job->mtx);
		return  JS_ERROR;
	}
	pthread_mutex_unlock(&Job->mtx);

	pthread_mutex_lock(&WP->mtx);

	if(WP->at_rest->nb+WP->at_work->nb < WP->max_workers) {
		addWorker(WP->at_rest, newWorker(WP->timeout, WP));
	}

	if( WP->pending_jobs->nb >0 || WP->at_rest->nb == 0) {
		tb_warn("add job %p in queue\n", Job);
		addJob(WP->pending_jobs, Job);
		Job->state = JS_QUEUED;
		rc = JS_QUEUED;
	} else {
		worker_t W = getWorker(WP->at_rest, NULL);
		pthread_mutex_lock(&W->mtx);
		Job->state = JS_ACCEPTED;
		W->request = START_JOB;
		W->cur_job = Job;
		addWorker(WP->at_work, W);

		tb_warn("job request accepted (worker %d)\n",W->id);
		pthread_cond_signal(&W->cond);
		pthread_mutex_unlock(&W->mtx);
		rc = JS_ACCEPTED;

	}
	pthread_mutex_unlock(&WP->mtx);

	return rc;
}


/*
struct synchro {
	pthread_mutex_t mtx;
	pthread_cond_t  cond;
};
typedef struct synchro *synchro_t;

synchro_t tb_Synchro() {
	synchro_t Sy = tb_xcalloc(1, sizeof(struct synchro));
	pthread_mutex_init(&Sy->mtx, NULL);
	pthread_cond_init(&Sy->cond, NULL);
	return Sy;
}

int waitSynchro(Synchro_t Sy, int sec, int usec) {
	pthread_mutex_lock(&Sy->mtx);
	pthread_cond_wait(&Sy->cond, &Sy->mtx);
}

void releaseSynchro(Synchro_t Sy) {
	pthread_mutex_unlock(&Sy->mtx);
}

void signalSynchro(Synchro_t Sy) {
	pthread_mutex_lock(&Sy->mtx);
	pthread_cond_signal(&Sy->cond);
	pthread_mutex_unlock(&Sy->mtx);
}



int waitComplete(job_list_t jobs, job_control_t flag, timeval tv) {
	if( flag == JC_ANY ) {
		for(tb_First(jobs); tb_Next(jobs); ) {
			if(tb_Value(jobs)->
*/
	

		

	
	
	

