//==================================================
// $Id: RW_lock.c,v 1.1 2004/05/12 22:04:51 plg Exp $
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

/**
 * @file RW_lock.c Basic Read an Write Locks with threading queue manager
 */

/**
 * @defgroup Locks Locks
 * Ressources locking manager
 * @todo tb_RLock_timed(), tb_WLock_timed()
 */


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "Toolbox.h"
#include "Objects.h"
#include "RW_Lock.h"

// CODE-ME: tb_RLock_timed()
// CODE-ME: tb_WLock_timed()

lock_manager_t __LM;
char *	_lock_mode[] = {"", "RLOCKER", "WLOCKER"};

static inline void * _P2p(tb_Object_t O) { return (O)? P2p(O) : NULL ; }

static pthread_once_t _lm_once = PTHREAD_ONCE_INIT;   



static void freeRWL(rw_lock_t RWL) {
	tb_xfree(RWL->sharers);
	tb_xfree(RWL->lockers);
	tb_xfree(RWL);
}


static int _share_list_push(share_list_t shl, tb_lock_t lock) {
	share_t sh = tb_xcalloc(1, sizeof(struct share));
	sh->lock = lock;
	if( shl->nb >0) {
		shl->last->next = sh;
		sh->prev = shl->last;
		shl->last = sh;
	} else {
		shl->first = shl->last = sh;
	}
	shl->nb++;
	return 1;
}

static tb_lock_t _share_list_shift(share_list_t shl) {
	share_t sh = shl->first;
	tb_lock_t lock = sh->lock;
	shl->first = sh->next;
	if( shl->first) shl->first->prev = NULL;
	shl->nb--;
	tb_xfree(sh);
	return lock;
}

static int _lock_list_remove(rw_lock_t RWL, int tid) {
	share_list_t shl = RWL->lockers;
	share_t sh;
	for(sh = shl->last; sh ; sh = sh->prev) {
		if(sh->lock->tid == tid) {
			if(sh->prev) sh->prev->next = sh->next;
			if(sh->next) sh->next->prev = sh->prev;
			if(sh == shl->first) shl->first= sh->next;
			if(sh == shl->last) shl->last= sh->prev;
			shl->nb--;
			if(sh->lock->access == TB_WLOCK) {
				RWL->writer_tid=0;
			}
			tb_xfree(sh->lock);
			tb_xfree(sh);
			return 1;
		}
	}
	return 0;
}

static int _sharer_to_locker(rw_lock_t RWL, int tid) {
	share_list_t shl = RWL->sharers;
	share_t sh;
	tb_lock_t L;
	for(sh = shl->first; sh ; sh = sh->next) {
		if(sh == NULL || sh->lock == NULL) abort();
		if(sh->lock->tid == tid) {
			if(sh->prev) sh->prev->next = sh->next;
			if(sh->next) sh->next->prev = sh->prev;
			if(sh == shl->first) shl->first= sh->next;
			if(sh == shl->last) shl->last= sh->prev;
			shl->nb--;
			L = sh->lock;
			tb_xfree(sh);
			_share_list_push(RWL->lockers, L);
			return 1;
		}
	}
	return 0;
}

static void dump_share_list(rw_lock_t RWL) {
	share_list_t shl = RWL->sharers;
	share_t sh;
	fprintf(stderr,"[%ld]Sharers:%d", pthread_self(), shl->nb);
	for(sh = shl->first; sh ; sh = sh->next) {
		fprintf(stderr,"[%ld:%s]", sh->lock->tid, _lock_mode[sh->lock->access]);
	}
	shl = RWL->lockers;
	fprintf(stderr,"\n[%ld]Lockers:%d", pthread_self(), shl->nb);
	for(sh = shl->first; sh ; sh = sh->next) {
		fprintf(stderr,"[%ld:%s]", sh->lock->tid, _lock_mode[sh->lock->access]);
	}
	fprintf(stderr, "\n");
}

static void init_LockManager() {
	__LM = tb_xcalloc(1, sizeof(struct lock_manager));
	pthread_mutex_init(&__LM->mtx, NULL);
	__LM->sharing = tb_Hash();
	tb_info("lock manager initialized\n");
}


static tb_lock_t newLock(int flag) {
	tb_lock_t L = tb_xcalloc(1, sizeof(struct tb_lock));
	pthread_mutex_init(&L->mtx, NULL);
	pthread_cond_init(&L->cond, NULL);
	L->tid = pthread_self();
	L->access = flag;

	return L;
}



static int wait_share(tb_lock_t L) {
	pthread_mutex_lock(&L->mtx);
	tb_info("wait_share: waiting for lock\n");
	pthread_cond_wait(&L->cond, &L->mtx);
	tb_info("wait_share: got the lock\n");
	pthread_mutex_unlock(&L->mtx);

	return 1;
}






/** Setup a Read lock on ressource
 *
 * If no lock have been created before, start the LockManager. Then create a read lock on pointer O : others can read, no-one can write.
 * @ingroup Locks
 */
int tb_Rlock(void * O) {
	
	tb_lock_t L;
	rw_lock_t RWL;
	String_t K = tb_String("%p", O);

	pthread_once(&_lm_once, init_LockManager);
	L = newLock(TB_RLOCK);
	pthread_mutex_lock(&__LM->mtx);

	RWL = (rw_lock_t)_P2p(tb_Get(__LM->sharing, S2sz(K)));
	if( RWL != NULL) {
		tb_info("Rlock: found RWL\n");
		pthread_mutex_lock(&RWL->mlock);
		pthread_mutex_unlock(&__LM->mtx);

		if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);

		if( RWL->writer_tid != 0 ) { // write lock set
			tb_info("Rlock: RWL already Wlocked\n");
			if( RWL->writer_tid != pthread_self() ) { // ?? is recursive locking considered harmful ?
				// need to block until writing is finished
				tb_info("Rlock: register for Rlocking\n");
				_share_list_push(RWL->sharers, L);
				pthread_mutex_unlock(&RWL->mlock);
				wait_share(L);
				pthread_mutex_lock(&RWL->mlock);
				_sharer_to_locker(RWL, pthread_self());
				pthread_mutex_unlock(&RWL->mlock);
			} else {
				tb_info("Rlock: recursive locking\n");
				_share_list_push(RWL->lockers, L);
				pthread_mutex_unlock(&RWL->mlock);
			}
		} else {
			if(RWL->sharers->nb > 0) {
				tb_info("Rlock: register for Rlocking\n");
				_share_list_push(RWL->sharers, L);
				pthread_mutex_unlock(&RWL->mlock);
				wait_share(L);
				pthread_mutex_lock(&RWL->mlock);
				_sharer_to_locker(RWL, pthread_self());
				pthread_mutex_unlock(&RWL->mlock);
			} else {
				tb_info("Rlock: not Wlocked : add Rlock\n");
				_share_list_push(RWL->lockers, L);
				pthread_mutex_unlock(&RWL->mlock);
			}
		}
	} else {
		tb_info("Rlock: RWL not already created : create and lock object [%S]\n", K);
		RWL = tb_xcalloc(1, sizeof(struct rw_lock));
		RWL->sharers = (share_list_t)tb_xcalloc(1, sizeof(struct share_list));
		RWL->lockers = (share_list_t)tb_xcalloc(1, sizeof(struct share_list));
		pthread_mutex_init(&RWL->mlock, NULL);
		_share_list_push(RWL->lockers, L);
		tb_Insert(__LM->sharing, tb_Pointer(RWL, freeRWL), S2sz(K));
		pthread_mutex_unlock(&__LM->mtx);
	}
	tb_Free(K);

	return 1;
}
		





/** Setup a Write lock on ressource
 *
 * If no lock have been created before, start the LockManager. Then create a write lock on pointer O : any other must wait for completion before to access to ressource.
 * @ingroup Locks
 */
int tb_Wlock(void * O) {
	
	tb_lock_t L;
	rw_lock_t RWL;
	String_t K = tb_String("%p", O);
	pthread_once(&_lm_once, init_LockManager);

	L = newLock(TB_WLOCK);
	pthread_mutex_lock(&__LM->mtx);

	RWL = (rw_lock_t)_P2p(tb_Get(__LM->sharing,S2sz(K)));
	if( RWL != NULL) {
		tb_info("Wlock: found RWL\n");
		pthread_mutex_lock(&RWL->mlock);
		pthread_mutex_unlock(&__LM->mtx);

		if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);

		if( RWL->writer_tid != 0 ) { // write lock set
			tb_info("Wlock: RWL already Wlocked\n");
			if( RWL->writer_tid != pthread_self() ) { // ?? is recursive locking considered harmful ?
				// need to block until writing is finished
				tb_info("Wlock: register for Wlock\n");
				_share_list_push(RWL->sharers, L);
				pthread_mutex_unlock(&RWL->mlock);
				wait_share(L);
				pthread_mutex_lock(&RWL->mlock);
				_sharer_to_locker(RWL, pthread_self());
				RWL->writer_tid= pthread_self();
				pthread_mutex_unlock(&RWL->mlock);
			} else {
				tb_info("Wlock: recursive Wlocking\n");
				_share_list_push(RWL->lockers, L);
				pthread_mutex_unlock(&RWL->mlock);
			}
		} else {
			if( RWL->lockers->nb > 0 ) { // some readers lock
				tb_info("Wlock: register for Wlock\n");
				_share_list_push(RWL->sharers, L);
				pthread_mutex_unlock(&RWL->mlock);
				wait_share(L);
				pthread_mutex_lock(&RWL->mlock);
				_sharer_to_locker(RWL, pthread_self());
				RWL->writer_tid= pthread_self();
				pthread_mutex_unlock(&RWL->mlock);
			} else {
				tb_info("Wlock: nobody locks : take Wlock\n");
				_share_list_push(RWL->lockers, L);
				RWL->writer_tid= pthread_self();
				pthread_mutex_unlock(&RWL->mlock);
			}
		}
	} else {
		tb_info("Wlock: RWL not already created : create and lock obj [%S]\n", K);
		RWL = tb_xcalloc(1, sizeof(struct rw_lock));
		RWL->sharers = (share_list_t)tb_xcalloc(1, sizeof(struct share_list));
		RWL->lockers = (share_list_t)tb_xcalloc(1, sizeof(struct share_list));
		pthread_mutex_init(&RWL->mlock, NULL);
		RWL->writer_tid= pthread_self();
		_share_list_push(RWL->lockers, L);
		tb_Insert(__LM->sharing, tb_Pointer(RWL, freeRWL), S2sz(K));
		pthread_mutex_unlock(&__LM->mtx);
	}
	tb_Free(K);
	
	return 1;
}
		




/** Unlocks a previously locked ressource
 *
 * Pending locking requests are awaken.
 * @ingroup Locks
 */
int tb_Unlock(void * O) {
	tb_lock_t L;
	rw_lock_t RWL;
	String_t K = tb_String("%p", O);

	pthread_once(&_lm_once, init_LockManager);
	pthread_mutex_lock(&__LM->mtx);

	RWL = (rw_lock_t)_P2p(tb_Get(__LM->sharing, S2sz(K)));
	if( RWL != NULL) {
		tb_info("Unlock: RWL found\n");
		pthread_mutex_lock(&RWL->mlock);
		pthread_mutex_unlock(&__LM->mtx);

		if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);

		_lock_list_remove(RWL, pthread_self());
		tb_info("Unlock: removed lock (%d lockers)\n", RWL->lockers->nb);

		if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);

		if(RWL->sharers->nb >0 && RWL->writer_tid == 0) {
			tb_info("Unlock: found %d waiting sharers\n", RWL->sharers->nb);
			while(RWL->sharers->nb >0) {
				L = RWL->sharers->first->lock;
				tb_info("Unlock: wanabee locker[%d] is a %s\n",L->tid, _lock_mode[L->access]);
				if(L->access == TB_RLOCK) {
					if( RWL->writer_tid == 0) {
						L = _share_list_shift(RWL->sharers);
						tb_info("Unlock: awake a Rlocker (%d)\n",L->tid);
						pthread_mutex_lock(&L->mtx);
						pthread_cond_signal(&L->cond);
						_share_list_push(RWL->lockers, L);
						pthread_mutex_unlock(&L->mtx);
						if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);
					} else {
						tb_info("Unlock: can't awake %d (%d Wlocks)\n", L->tid, RWL->writer_tid);
						break;
					}
				} else { // exclusive access requested
					if( RWL->writer_tid == 0 && RWL->lockers->nb == 0) {
						L = _share_list_shift(RWL->sharers);
						tb_info("Unlock: awake a Wlocker (%d)\n",L->tid);
						pthread_mutex_lock(&L->mtx);
						pthread_cond_signal(&L->cond);
						_share_list_push(RWL->lockers, L);
						RWL->writer_tid = L->tid;
						pthread_mutex_unlock(&L->mtx);
						if(tb_errorlevel >= TB_NOTICE) dump_share_list(RWL);
					} else {
						tb_info("Unlock: can't awake %d (%d lockers)\n", L->tid, RWL->lockers->nb);
						break;
					}
				}
			}
		}

		if(RWL->sharers == 0 && RWL->lockers == 0) {
			if(pthread_mutex_lock(&__LM->mtx) != EBUSY) {
				pthread_mutex_unlock(&RWL->mlock);
				tb_Remove(__LM->sharing, S2sz(K));
				pthread_mutex_unlock(&__LM->mtx);
			}
		} else {
			pthread_mutex_unlock(&RWL->mlock);
		}
	} else {
		tb_info("Unlock: RWL[%S] not found\n", K);
		abort();
	}
	tb_Free(K);

	return 1;
}


