//========================================================================
// 	$Id: Server.c,v 1.2 2004/07/01 21:38:23 plg Exp $
//========================================================================
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

//fixme: the way to pass user_data to callback is broken : callback should be [int cb(So,user_data)] 

#ifndef _REENTRANT
#  define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#  define _POSIX_PTHREAD_SEMANTICS
#endif


// #define WITH_SOCK_ACL FIXME: not ready for prime-time + fix bpt_newNode leak first

#include "tb_global.h"

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <unistd.h>    
#include <signal.h>

#include <arpa/inet.h>
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#  include <strings.h>
#endif  
#include <stddef.h>
#include <stdlib.h>

#include "Socket.h"
#ifdef WITH_SOCK_ACL
#include "Sock_ACL.h"
#endif
#include "Toolbox.h"
#include "Composites.h"
#include "Memory.h"
#include "Error.h"


static retcode_t tb_initSockServer_IP        (Socket_t O, void *callback, void *cb_args);
static retcode_t tb_initSockServer_UNIX      (Socket_t O, void *callback, void *cb_args);
static void *    tb_Accept_IP                (Socket_t O);

static inline void * _P2p(tb_Object_t O) { return (O)? P2p(O) : NULL ; }


void   *tb_getServArgs     (Socket_t O) { // fixme: brok brok brok !
	no_error;
	if(!TB_VALID(O, TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return NULL;
	}
	if(XSock(O)->server) {
		return XServer(O)->args;
	}
	return NULL;
}

int tb_getSockFD     (Socket_t O) {
	if(!TB_VALID(O, TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	return XSock(O)->sock;
}

retcode_t tb_initServer(Socket_t O, void *callback, void *cb_args) {
	if(!TB_VALID(O, TB_SOCKET))  {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(XSock(O)->addr_family == TB_IP) {
		return tb_initSockServer_IP(O, callback, cb_args);
	} else if(XSock(O)->addr_family == TB_UNIX) {
		return tb_initSockServer_UNIX(O, callback, cb_args);
	}
	return TB_ERR;
}




//Fixme: either close ASAP, or wait for all threads to be done
retcode_t tb_stopServer     (Socket_t O) { //, int close_mode) {
	sock_server_t ms;
	if(!TB_VALID(O, TB_SOCKET))  {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(XSock(O)->server == NULL) {
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR; 
	}
	if( tb_getSockStatus(O) == TB_LISTENING ) {
		ms = XServer(O);
		pthread_mutex_lock(&(ms->shutdown_mtx));
		XSock(O)->status = TB_SHUTDOWN;
		tb_notice("tb_stopServer: now waiting for server to stop ...\n");
		pthread_cond_wait(&(ms->shutdown_cnd), &(ms->shutdown_mtx));
		pthread_mutex_unlock(&(ms->shutdown_mtx));
		tb_notice("tb_stopServer: done.\n");
		return TB_OK;
	}
	return TB_KO;
}

retcode_t tb_setServMAXTHR     (Socket_t O, int max) {
	if(!TB_VALID(O, TB_SOCKET))  {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(XSock(O)->server == NULL) {
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR; 
	}
	XServer(O)->max_threads = max;

	return TB_OK;
}

int tb_getServMAXTHR     (Socket_t O) {
	if(!TB_VALID(O, TB_SOCKET))  {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(XSock(O)->server == NULL)  {
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR; 
	}
	return (XServer(O)->max_threads);
}



static retcode_t tb_initSockServer_IP(Socket_t O, 
																		void *callback, // int(*)(Socket_t)
																		void *cb_args) {
  
  struct sockaddr_in name;
  int                myProto;
  int                val = 1;
	sock_members_t       S;
	sock_server_t      Serv;

	struct hostinf *hostinf;
	struct hostent *h;

  no_error;
	if(! TB_VALID(O, TB_SOCKET)) {
		tb_error("tb_initSockServer: Not a Socket_t\n");
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}

	if( tb_getSockStatus(O) != TB_UNSET ) {
		tb_error("tb_initSockServer: socket yet in use !\n");
		set_tb_errno(TB_ERR_IN_USE);
		return TB_ERR;
	}

	S = XSock(O);

  S->status   = TB_UNSET;

	Serv = S->server  = (sock_server_t)tb_xcalloc(1, sizeof(struct sock_server));
	
	Serv->callback    = callback;
	Serv->args        = cb_args;
	Serv->max_threads = 100; // default
#ifdef WITH_SOCK_ACL
	// Serv->acl->use_acl defaults to 0 until you mess with acl
	Serv->acl         = _new_sock_acl();
	pthread_mutex_init(&(Serv->acl->mtx), NULL);
#endif

	pthread_mutex_init(&(Serv->shutdown_mtx), NULL);
	pthread_cond_init(&(Serv->shutdown_cnd), NULL);

  myProto        = (S->proto == TB_TCP) ? SOCK_STREAM : SOCK_DGRAM;
  
  S->sock = socket(PF_INET, myProto, 0);
	if(tb_errorlevel >= TB_NOTICE) {
		if(S->sock < 0)	{ 
			tb_error("Server_init: socket %s\n", strerror(errno)); 
			set_tb_errno(TB_ERR_SOCKET_FAILED);
			return TB_ERR; 
		}
	}
  // allow reuse of socket (even if yet bound by defunct process)
  setsockopt(S->sock, SOL_SOCKET, SO_REUSEADDR , (const void *)&val, sizeof(int));

	name.sin_family      = AF_INET;
	name.sin_port        = htons(S->port);

	// if user specifyied an address to bind to, try it, else bind to '*.port'
	if( strlen(S->name) >0 ) { 
		tb_warn("will try to bind sock to <%s>\n", S->name);
		if(( hostinf = gethost_sin_addr(S->name)) == NULL) {
			tb_error("tb_initSockServer: unknown host <%s>\n", S->name);
			set_tb_errno(TB_ERR_INVALID_HOSTNAME);
			return TB_ERR;
		}
		h = hostinf->he;
  
		name.sin_addr.s_addr = ((struct in_addr *)(h->h_addr))->s_addr;
		hostinf_free(hostinf);
	} else {
		name.sin_addr.s_addr = htonl(INADDR_ANY);
	}

  if(bind( S->sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
    tb_error("Server_init: bind %s\n", strerror(errno)); 
		set_tb_errno(TB_ERR_BIND_FAILED);
		return TB_ERR;
  }
	if( S->proto == TB_TCP ) {
		if(listen( S->sock, 10 )) {
			tb_error("Server_init: listen %s\n", strerror(errno)); 
			set_tb_errno(TB_ERR_LISTEN_FAILED);
			return TB_ERR;
		}
	}
  S->status = TB_LISTENING;

  return TB_OK;
}


static retcode_t tb_initSockServer_UNIX(Socket_t O, void *callback, void *cb_args) {
  
  int                val = 1;
	int                myProto;
	sock_members_t       S;
	sock_server_t      Serv;

	struct sockaddr_un name;
	size_t size;
  
	if(! TB_VALID(O, TB_SOCKET)) {
		tb_error("tb_initSockServer: Not a Socket_t\n");
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}

	if( tb_getSockStatus(O) != TB_UNSET ) {
		tb_error("tb_initSockServer: socket yet in use !\n");
		set_tb_errno(TB_ERR_IN_USE);
		return TB_ERR;
	}
	S = XSock(O);

  S->status   = TB_UNSET;
	unlink(S->name);

	Serv = S->server  = (sock_server_t)tb_xcalloc(1, sizeof(struct sock_server));
	
	Serv->callback    = callback;
	Serv->args        = cb_args;
	Serv->max_threads = 100; // default

  myProto        = (S->proto == TB_TCP) ? SOCK_STREAM : SOCK_DGRAM;
  
  S->sock = socket(AF_UNIX, myProto, 0);
  if (tb_errorlevel >= TB_NOTICE) {
		if(S->sock < 0)	{ 
			tb_error("Server_init: socket %s\n", strerror(errno)); 
			set_tb_errno(TB_ERR_SOCKET_FAILED);
			return TB_ERR; 
		}
	}
  // allow reuse of socket (even if yet bound by defunct process)
  setsockopt(S->sock, SOL_SOCKET, SO_REUSEADDR , (const void *)&val, sizeof(int));

	name.sun_family = AF_UNIX;
	strncpy (name.sun_path, S->name, sizeof (name.sun_path));

	// The size of the address is
	// the offset of the start of the filename,
	// plus its length,
	// plus one for the terminating null byte.
	// Alternatively you can just do:
	// size = SUN_LEN (&name);
	
	size = (offsetof (struct sockaddr_un, sun_path)
					+ strlen (name.sun_path) + 1);
	if (bind (S->sock, (struct sockaddr *) &name, size) < 0) {
    tb_error("Server_init: bind %s\n", strerror(errno)); 
		set_tb_errno(TB_ERR_BIND_FAILED);
		return TB_ERR;
  }
	if( S->proto == TB_TCP ) {
		if(listen( S->sock, 10 )) {
			tb_error("Server_init: listen %s\n", strerror(errno)); 
			set_tb_errno(TB_ERR_LISTEN_FAILED);
			return TB_ERR;
		}
	}
  S->status = TB_LISTENING;

  return TB_OK;
}



void *tb_Accept( void *Arg ) {  // arg is void * for use w/ pthread_create
	Socket_t O    = (Socket_t) Arg;

	no_error;

	if(! TB_VALID(O, TB_SOCKET)){
		tb_warn("invalid object\n");
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return NULL;
	}
	
	switch( XSock(O)->addr_family ) {
	case TB_IP:
		return tb_Accept_IP(O);
#ifdef WITH_XTI
	case TB_X25:
		return tb_Accept_x25(O);
#endif
	}
	set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
	return NULL;
}

static void *tb_Accept_IP( Socket_t O ) {
  struct sockaddr incoming;
  fd_set set ;
  struct timeval tps ;
  int s = -1;
  spawn_args    *sp_args = NULL;
	Socket_t A;
	sock_members_t  S;
	sock_server_t Srv;

  socklen_t        len               = sizeof(incoming);
  pthread_t      * pt                = NULL;
  pthread_cond_t * exit_cond;
  int              actual_threads_nb = 0;
	int nothing_to_do                  = 1;
#ifdef WITH_SSL
	SSL            * ssl_cx            = NULL;
  BIO* sbio=NULL,* dbio;
	int  use_ssl                       = 0;
#endif

	if(XSock(O)->server == NULL) {
		tb_warn("not/uninitilized server\n");
		set_tb_errno(TB_ERR_BAD);
		return NULL;
	}
	if(XServer(O)->callback == NULL) {
		tb_warn("not/uninitilized server callback\n");
		set_tb_errno(TB_ERR_BAD);
		return NULL;
	}
#ifdef WITH_SSL
	if(XSsl(O) != NULL) use_ssl = 1;
#endif	
  exit_cond         = tb_xmalloc(sizeof(pthread_cond_t));
	pthread_cond_init(exit_cond, NULL);

  S   = XSock(O);
	Srv = XServer(O);

	set_nonblock_flag(S->sock, 1);

  Srv->nb_t_mtx = (pthread_mutex_t *)tb_xmalloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(Srv->nb_t_mtx, NULL);

  if(tb_getSockStatus(O) == TB_LISTENING) {

		tb_notice("tb_Accept[%d]: start accepting cnx on socket %d\n", 
							S->port, tb_getSockFD(O));

	restart:
		while( 1 ) {

			nothing_to_do = 1;

			while ( nothing_to_do ) {
				int rc;
				FD_ZERO(&set);
				FD_SET(S->sock, &set);
				
				tps.tv_sec = 0;
				tps.tv_usec = 5000;
 				rc = select(S->sock+1, &set, NULL, NULL, &tps);
				if(tb_getSockStatus(O) == TB_SHUTDOWN) rc = -2;

				switch (rc)
					{
					case -2: // shutdown requested
						// free what need to be freed
						{
							sock_server_t ms = XServer(O);
							tb_notice("tb_Accept[%d]: stopping server (by request)\n", S->port);

							pthread_mutex_lock(Srv->nb_t_mtx);
							while( actual_threads_nb > 0 ) {
								tb_notice("tb_Accept[%d]: shutdown in progress: still %d thread(s) working\n",
													S->port, actual_threads_nb);
								pthread_cond_wait( exit_cond, Srv->nb_t_mtx );
							}
							pthread_mutex_unlock(Srv->nb_t_mtx);
							tb_xfree( exit_cond );
							pthread_mutex_lock(&(ms->shutdown_mtx));
							pthread_cond_signal(&(ms->shutdown_cnd));
							pthread_mutex_unlock(&(ms->shutdown_mtx));
						}
						return NULL;
					case -1:
						if( errno == EINTR ) break;
						tb_xfree( exit_cond );
						goto bad_event;
					case 0: // timed-out : still no incommers
						break;
				default:

					s = accept(S->sock, (struct sockaddr *)&incoming, &len);
					if( s < 0 ) {
						tb_xfree( exit_cond );
						return NULL;
					} else {
						nothing_to_do = 0;
					}
					break;
				}
			}

			if(tb_errorlevel >= TB_NOTICE) {
				if(tb_errorlevel == TB_DEBUG) tb_Dump(sp_args->So);
      
				if(XSock(O)->proto == TB_TCP) {
					if(XSock(O)->addr_family == TB_IP) {
						tb_warn("tb_Accept[%d]: incoming connection from %s:%d -> %d\n", 
										 S->port,
										 (char *)inet_ntoa(((struct sockaddr_in *)(&incoming))->sin_addr),
										 ((struct sockaddr_in *)(&incoming))->sin_port, s);
					} else {
						tb_warn("tb_Accept[%d]: incoming connection from %s -> %d\n", 
										 S->port,
										 S->name,
										 s);
					}
				} else {
					tb_notice("tb_Accept: incoming connection -> %d\n", s);
				}
			}
      

			// ACL validation take place here
#ifdef WITH_SOCK_ACL
			if( Srv->acl->use_acl ) {
			 	pthread_mutex_lock(&Srv->acl->mtx);
				if(! check_sock_acl(Srv->acl, &incoming)) {
					pthread_mutex_unlock(&Srv->acl->mtx);
					tb_profile("ACL done\n");
					tb_warn("tb_Accept[%d]: ACL access denied to %s:%d -> %d\n", 
									 S->port,
									 (char *)inet_ntoa(((struct sockaddr_in *)(&incoming))->sin_addr),
									 ((struct sockaddr_in *)(&incoming))->sin_port, s);
					close(s);
					goto restart;
				}
				pthread_mutex_unlock(&Srv->acl->mtx);
				tb_notice("tb_Accept[%d]: ACL access granted to %s:%d -> %d\n", 
								 S->port,
								 (char *)inet_ntoa(((struct sockaddr_in *)(&incoming))->sin_addr),
								 ((struct sockaddr_in *)(&incoming))->sin_port, s);
			}
#endif

#ifdef WITH_SSL
			if( use_ssl ) {
				if (!(ssl_cx = (SSL *)SSL_new(XSsl(O)->ctx))) {
					tb_error("tb_Accept: Cannot create new SSL connection\n");
					ERR_print_errors_fp(stderr);
					close(s);
					SSL_free(ssl_cx);
					goto restart;
				}
				tb_info("ssl cx initialized\n");


				if (tb_errorlevel < 6) { 
					SSL_set_fd(ssl_cx, s);
				} else {
					if (!(sbio=BIO_new_socket(s, BIO_NOCLOSE))) {
						tb_error("tb_Accept: Cannot create new socket BIO (SSL)\n");
						ERR_print_errors_fp(stderr);
						close(s);
						SSL_free(ssl_cx);
						goto restart;
					}
					SSL_set_bio(ssl_cx,sbio,sbio);
					ssl_cx->debug=1;
					dbio=BIO_new_fp(stderr,BIO_NOCLOSE);
					BIO_set_callback(sbio,(void *)bio_dump_cb);
					BIO_set_callback_arg(sbio,dbio);
				}

			
				if( SSL_accept(ssl_cx) == -1) {
					tb_error("cx is not SSL conforming - aborting connexion -\n");
					close(s);
					SSL_free(ssl_cx);
					goto restart;
				}
			}
#endif

      pt = (pthread_t *)tb_xmalloc(sizeof(pthread_t));
      
      sp_args    = (spawn_args *)   tb_xmalloc(sizeof(spawn_args)); 
      
			A                           = tb_Clone(O);
      sp_args->So                 = A;
      sp_args->Parent             = O;

      sp_args->mutex              = Srv->nb_t_mtx;
      sp_args->cond               = exit_cond;
      sp_args->nb                 = &actual_threads_nb;
      XSock(A)->sock              = s;
      XSock(A)->status            = TB_CONNECTED;
#ifdef WITH_SSL
			if(use_ssl) XSsl(A)->cx    = ssl_cx;
#endif
      pthread_mutex_lock(Srv->nb_t_mtx);
			if(actual_threads_nb == Srv->max_threads ) {
				while( actual_threads_nb == Srv->max_threads ) {
					tb_warn("tb_Accept[%d]: max thread limit reached (%d). Waiting for a free...\n", 
									 S->port, actual_threads_nb);
					pthread_cond_wait( exit_cond, Srv->nb_t_mtx );
				}
				tb_warn("tb_Accept[%d]: (max thread blocking end) resuming operations\n", 
								 S->port);
			}

      actual_threads_nb++;
      pthread_mutex_unlock(Srv->nb_t_mtx);
#ifdef WITH_SSL
			if(use_ssl ) {
				if(tb_errorlevel >= TB_NOTICE ) ssl_barf_out(A);
				if((XSsl(A)->peer = SSL_get_peer_certificate(XSsl(A)->cx)) != NULL) {
					char buf[BUFSIZ];
					X509_NAME_oneline(X509_get_subject_name(XSsl(A)->peer),buf,BUFSIZ);
					tb_info("Peer cert subject=%s\n", buf);
					X509_NAME_oneline(X509_get_issuer_name(XSsl(A)->peer),buf,BUFSIZ);
					tb_info("Peer cert issuer=%s\n", buf);
					X509_free(XSsl(A)->peer); // fixme: should be done in destructor, so we could access it from callback
				} else {
					tb_info("client w/o cert\n");
				}
			}
#endif
      pthread_create(pt, NULL, &spawner, sp_args);
			if(tb_errorlevel >= TB_NOTICE) {
				tb_notice("tb_Accept[%d]: starting a new thread[%d] (#%d/#%d)\n", 
								 S->port, *pt, actual_threads_nb, Srv->max_threads);
			}

			pthread_detach(*pt);
      tb_xfree(pt);

    }
		if(errno == EINTR) goto restart;

	bad_event:
    tb_warn("tb_Accept[%d]: no more listening on %d\n", S->port, S->sock);
    tb_warn("tb_Accept[%d]: rc=%d (%d, %s)\n", S->port, s, errno, strerror(errno));
  } else {
    tb_warn("tb_Accept[%d] : ServerIP : not listening :-(\n", S->port);
  }

	return NULL;
}







void *spawner(void *arg) {
  pthread_mutex_t *mutex  = ((spawn_args *)arg)->mutex; 
  pthread_cond_t  *cond   = ((spawn_args *)arg)->cond; 
  int t, *nb              = ((spawn_args *)arg)->nb;
	Socket_t S              = (Socket_t )((spawn_args *)arg)->So;
  int (*fnc)(Socket_t)    = (XServer(S))->callback;
  int rc;
#ifdef WITH_SOCK_ACL
	Socket_t Parent         = (Socket_t )((spawn_args *)arg)->Parent;
	sock_server_t Srv       = XServer(Parent);
	freq_acl_t F;
	socklen_t slen = sizeof(saddr);
	char *name = NULL;;
	struct sockaddr saddr;
#endif

  t = *nb;
  
  tb_info( "tb_Accept: spawn callback[fd:%d]\n", tb_getSockFD(S));
  rc = (*fnc)(S);

  pthread_mutex_lock(mutex);
  (*nb)--;
  pthread_cond_signal(cond);

#ifdef WITH_SOCK_ACL
	if( Srv->acl->use_acl ) {
		if(getsockname(XSock(S)->sock, &saddr, &slen) == 0) {
			name = (char *)inet_ntoa(((struct sockaddr_in *)(&saddr))->sin_addr);
		}
		pthread_mutex_lock(&Srv->acl->mtx);
		Srv->acl->global_freq_acl->nb_simult--;
		if( Srv->acl->max_host_simult) {
			F = _P2p(tb_Get(Srv->acl->host_freq_acl, name));
			if( F ) {
				if( -- F->nb_simult <=0 ) {
					/*
						if( F->timestamp < time(NULL)) {
						tb_Remove(Srv->acl->host_freq_acl, name); // fixme: move to "removable hash" and destroy with respect to generation (older get destroyed first)
						}
					*/
				}
				tb_warn("spawner end: nb_simult/host = %d\n", F->nb_simult);
			}
		}
		pthread_mutex_unlock(&Srv->acl->mtx);
	}
#endif

  pthread_mutex_unlock(mutex);
  tb_info( "tb_Accept: callback[fd:%d] exits w/ rc=%d\n", tb_getSockFD(S), rc);
  tb_xfree(arg);
	tb_Free(S);
  
	pthread_exit(0);

  return NULL;
}




