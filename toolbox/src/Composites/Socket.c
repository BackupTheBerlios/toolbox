//========================================================================
// 	$Id: Socket.c,v 1.1 2004/05/12 22:04:50 plg Exp $	
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

/**
 * @file Socket.c
 */

/**
 * @defgroup Socket Socket_t
 * OO managenemt of networking endpoints
 * @ingroup Composite
 */

#ifndef _REENTRANT
#  define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#  define _POSIX_PTHREAD_SEMANTICS
#endif


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
#include <netinet/tcp.h> // check if really exists under glibc <= 2.0
#include <unistd.h>    
#include <signal.h>

#ifdef AIX
#  include <sys/mbuf.h>
#  include <netinet/if_ether.h>
#  include <net/if_dl.h>
#endif
#include <arpa/inet.h>
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#  include <strings.h>
#endif  
#include <stddef.h>
#include <stdlib.h>

#include "Toolbox.h"
#include "Socket.h"
#include "Composites.h"
#include "Memory.h"
#include "Error.h"
#include "tb_ClassBuilder.h"

#ifdef AIX
#define PF_LOCAL AF_UNIX
#endif

static int     tb_initSocket_UNIX          (Socket_t O);
static int     tb_initSocket_IP            (Socket_t O);

inline sock_members_t XSock(Socket_t S) {
	return (sock_members_t)((__members_t)tb_getMembers(S, TB_SOCKET))->instance;
}

inline sock_server_t XServer(Socket_t S) {
	return (sock_server_t)((sock_members_t)
												 ((__members_t)tb_getMembers(S, TB_SOCKET))->instance)->server;
}

inline srv_acl_t XAcl(Socket_t S) {
	return (srv_acl_t)((sock_server_t)
										 ((sock_members_t)
											((__members_t)tb_getMembers(S, TB_SOCKET))->instance)->server)->acl;
}

inline sock_ssl_t     XSsl(Socket_t S) {
	return (sock_ssl_t)((sock_members_t)
											((__members_t)tb_getMembers(S, TB_SOCKET))->instance)->ssl;
}

void __build_socket_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_sock_new);
	tb_registerMethod(OID, OM_FREE,         tb_sock_free);
	tb_registerMethod(OID, OM_CLONE,        tb_sock_clone);
	tb_registerMethod(OID, OM_DUMP,         tb_sock_dump);
	tb_registerMethod(OID, OM_CLEAR,        tb_sock_clear);
}


Socket_t dbg_tb_socket(char *func, char *file, int line, int mode, char *name, int port) { 
	set_tb_mdbg(func, file, line);
	return tb_Socket(mode, name, port);
}


/** Socket_t constructor.
 * \ingroup Socket
 * Create a Socket object. No connexion attempts are done yet.
 *
 * Socket_t encapsulate in one object many commonly used socket features. Endpoint parameters are stored internally, allowing easy reconnection ; timeouts, number of retries, lap between retries are internal members too. Connection mode can be any combination of networking families and protocols in TCP/UDP and IP/UNIX.
 * Socket_t can either be used for client or server access.
 * See socket_test.c, server_test.c for examples.
 *
 * @param mode
 * \arg TB_TCP_IP : TCP/IP standard stream connection
 * \arg TB_TCP_UX : TCP/UNIX local stream connection
 * \arg TB_UDP_IP : UDP/IP packet endpoint (connectionless)
 * \arg TB_UDP_UX : UDP/UNIX packet endpoint over unix socket
 *  @param ... char *name, int port if ip, char *path if unix domain
 * @return newly allocated Socket_t
 *
 * Socket_t internal status is of type TB_SOCKSTATUS:
 * - TB_BROKEN
 * - TB_UNSET
 * - TB_DISCONNECTED
 * - TB_CONNECTED
 * - TB_LISTENING
 * - TB_TIMEDOUT
 * 
 *
 * Methods:
 * - tb_Connect        : try to establish client connection
 * - tb_setSockNoDelay : disable Nagle algorithme
 * - tb_initServer     : prepare a listening server
 * - tb_getServArgs    : access to server args
 * - tb_getSockFD      : access to internal socket (legacy C fd)
 * - tb_setServMAXTHR  : setup max allowed thread for a server
 * - tb_getServMAXTHR  : get max allowed thread for a server
 * - tb_setSockTO      : setup socket timeout
 * - tb_getSockTO      : get socket timeout
 * - tb_getSockStatus  : get socket internal status
 * - tb_Accept         : spawn thread on incoming connection
 * - tb_readSock       : socket read with timeout
 * - tb_writeSock      : socket write with timeout
 *
 * @see Object_t, Composites tb_Connect, tb_setSockNoDelay, tb_initServer, tb_getServArgs, tb_getSockFD, tb_setServMAXTHR, tb_getServMAXTHR, tb_setSockTO, tb_getSockTO, tb_getSockStatus, tb_Accept, tb_readSock, tb_readSockLine, tb_writeSock
 */
Socket_t tb_Socket(int mode, ...) {
	char *name;
	int port;
	va_list ap;
	va_start(ap, mode);

	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	
	if(mode & TB_IP) {
		name = va_arg(ap, char *);
		port = va_arg(ap, int);
		return tb_sock_new(tb_newParent(TB_SOCKET), mode, name, port);
	} else if(mode & TB_UNIX) {
		name = va_arg(ap, char *);
		return tb_sock_new(tb_newParent(TB_SOCKET), mode, name, 0);
	} 

	tb_trace(TB_CRIT, "tb_Socket: unknown protocol family\n");
	return NULL;
}



void tb_sock_dump(Socket_t O, int level) {
  int i; 
	socklen_t len = sizeof(struct sockaddr);
	struct sockaddr_in sa;
	char *proto;
	char *sock_status[] ={ "BROKEN", 
												 "UNSET", 
												 "DISCONNECTED", 
												 "CONNECTED", 
												 "LISTENING", 
												 "TIMEOUT" };
	sock_members_t members = XSock(O);


  for(i = 0; i<level; i++) fprintf(stderr, " ");
	switch( members->proto ) {
	case TB_TCP:		
		if( members->addr_family == TB_IP ) {
			proto = "TCP/IP";
		} else {
			proto = "TCP/UNIX";
		}
		break;
	case TB_UDP:		
		if(members->addr_family == TB_IP ) {
			proto = "UDP/IP";
		} else {
			proto = "UDP/UNIX";
		}
		break;
	default: proto = "UNKNOWN/ERROR";
	}

	if(members->status == TB_CONNECTED) {
		if(members->addr_family == TB_IP) {
			getpeername( members->sock, (struct sockaddr *)&sa, &len); 
			fprintf(stderr, "<TB_SOCKET ADDR=\"%p\" PROTO=\"%s\" FD=\"%d\" STATUS=\"%s\" PEER=\"%s:%d\" />\n",
							O,
							proto, 
							members->sock, 
							sock_status[members->status],
							(char *)inet_ntoa(sa.sin_addr),
							sa.sin_port
							);
		} else {
			fprintf(stderr, "<TB_SOCKET ADDR=\"%p\" PROTO=\"%s\" FD=\"%d\" STATUS=\"%s\" PEER=\"%s\" />\n",
							O,
							proto, 
							members->sock, 
							sock_status[members->status],
							members->name
							);
		}
	}	else if(members->status == TB_LISTENING) {
		fprintf(stderr, "<TB_SOCKET ADDR=\"%p\" PROTO=\"%s\" FD=\"%d\" STATUS=\"%s\" CALLBACK=\"%p\" />\n",
						O,
						proto, 
						members->sock, 
						sock_status[members->status],
						XServer(O)->callback
					);
	}	else {
		fprintf(stderr, "<TB_SOCKET ADDR=\"%p\" PROTO=\"%s\" FD=\"%d\" STATUS=\"%s\" />\n",
						O,
						proto, 
						members->sock, 
						sock_status[members->status]
					);
	}
	//	TB_UNLOCK(O);
}


Socket_t tb_sock_new(tb_Object_t O, int Proto, char *name, int port) {
	sock_members_t m;

	O->isA   = TB_SOCKET;
  O->members->instance = (sock_members_t)tb_xcalloc(1, sizeof(struct sock_members));
	m = XSock(O);
	m->sock        = -1;
	if(Proto & TB_UNIX) {
		m->addr_family = TB_UNIX;
	} else 	if(Proto & TB_IP) {
		m->addr_family = TB_IP; // default
 	}

	if(Proto & TB_UDP) {
		m->proto       = TB_UDP;
	} else {
		m->proto       = TB_TCP;  // default
	}

	m->name        = tb_xstrdup(name);
	m->port        = port;
	m->status      = TB_UNSET;
	m->to_sec      = 1;
	m->to_usec     = 0;
	m->retries     = 3;
	m->retry_lap   = 2;
	m->server      = NULL;
	m->buffer      = NULL;

	if(fm->dbg) fm_addObject(O);

	return O;
}



/** tb_Socket init and connect.
 * \ingroup Socket
 *Tries to establish connection, using Socket_t internal parameters. 
 * Sets internal status (see tb_getSockStatus);
 *
 * @param So target Socket_t
 * @param TO timeout in seconds before connect fail
 * @param nb_tries number of retries before fail
 * @return RETCODE
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if So not a TB_SOCKET
 * - TB_ERR_INVALID_PROTOCOL if So's protocol is unknown
 * - TB_ERR_SOCKET_FAILED if socket(2) failed
 * - TB_ERR_SELECT_FAILED if select(2) failed
 * - TB_ERR_CONNECT_FAILED if all connect attempts failed
 *
 * @see tb_Socket, tb_getSockStatus
 */
int tb_Connect(Socket_t So, int TO, int nb_tries) {
	sock_members_t m;

	if(!tb_valid(So, TB_SOCKET, __FUNCTION__)) return TB_ERR;
	m = XSock(So);

	m->to_sec  = TO;
	m->retries = nb_tries;

	if(m->addr_family == TB_IP) {
		return tb_initSocket_IP(So);
	} else if(m->addr_family == TB_UNIX) {
		return tb_initSocket_UNIX(So);
	} 
	set_tb_errno(TB_ERR_INVALID_PROTOCOL);

	return TB_ERR;
}


static int tb_initSocket_UNIX(Socket_t O) {
	sock_members_t           m;
  struct sockaddr_un       sockaddr;
  int                      myProto, ret, retries = 0;
	socklen_t                rc;
  struct timeval           timeout;
	size_t                   size;

	m = XSock(O);
	
	sockaddr.sun_family = AF_UNIX;
	strcpy(sockaddr.sun_path, m->name);
	size = strlen(sockaddr.sun_path) + sizeof(sockaddr.sun_family); 

  myProto = (m->proto == TB_TCP) ? SOCK_STREAM : SOCK_DGRAM;
	if((m->sock = socket(AF_UNIX, myProto, 0)) == -1) {	
		tb_error("tb_initSocket_UNIX: socket error <%s:%d> %d: %s\n", 
						 m->name, m->port, errno, strerror(errno));
		m->status = TB_UNSET; 
		set_tb_errno(TB_ERR_SOCKET_FAILED);
		return TB_ERR; 
	}

	while( retries++ < m->retries) {
		set_nonblock_flag(m->sock, 1);
	
		if((connect(m->sock, (struct sockaddr *)&sockaddr, size)) == -1) 
			{ 
				if(errno == EINPROGRESS || 
					 errno == EWOULDBLOCK ||
					 errno == EALREADY    ||
					 errno == EISCONN ) 
					{
						FD_SET(m->sock, &(m->set));
						timeout.tv_sec  = 1;
						timeout.tv_usec = m->to_usec;
						rc = select(m->sock +1, NULL, &(m->set), NULL, &timeout);
						switch( rc ) {
						case -1:
							m->status = TB_BROKEN;
							tb_error("tb_initSocket_UNIX: #%d/%d select: %d: %s\n", 
											retries, m->retries, errno, strerror(errno));
							close(m->sock);
							set_tb_errno(TB_ERR_SELECT_FAILED);
							return TB_ERR;

						case 0: //timeout
							m->status = TB_TIMEDOUT ;
							tb_warn("tb_initSocket_UNIX: #%d/%d <%s> connect timeout\n", 
											retries, m->retries, m->name);
							continue;

						default:
							rc = sizeof(ret);
							getsockopt(m->sock, SOL_SOCKET, SO_ERROR, (void *)&ret, &rc);
							if(ret == 0) {
								set_nonblock_flag(m->sock, 0);
								m->status = TB_CONNECTED;
								return TB_OK;
							}
						}
					} 
				else 
					{
						tb_warn("tb_initSocket_UNIX: #%d/%d <%s> %d: %s\n", 
										retries, m->retries, m->name, errno, strerror(errno));
						continue;
					}
			} 
		else 
			{
				set_nonblock_flag(m->sock, 0);
				m->status = TB_CONNECTED;
				return TB_OK;
			}
		if(retries < m->retries) sleep(m->retry_lap);
	}
	tb_error("tb_initSocket_UNIX: can't connect <%s>\n", m->name);

	set_tb_errno(TB_ERR_CONNECT_FAILED);

	return TB_ERR;
}



static int tb_initSocket_IP(Socket_t O) {
	sock_members_t           S;
  struct sockaddr_in     sockaddr, *s;
	struct hostinf       * hostinf;
	struct hostent       * h;
  int                    n, port, myProto, ret, retries = 0;
	socklen_t              rc;
  struct timeval         timeout;
	char                 * hostname;

	s = (struct sockaddr_in *)&sockaddr;

	S = XSock(O);
	port = S->port;
  FD_ZERO(& (S->set));
  S->status = TB_UNSET;

  myProto = (S->proto == TB_TCP) ? SOCK_STREAM : SOCK_DGRAM;
	
	hostname = (*S->name == 0) ? "localhost" : S->name;
	if(( hostinf = gethost_sin_addr(hostname)) == NULL) {
		tb_error("tb_initSocket_IP: unknown host <%s>\n", hostname);
		set_tb_errno(TB_ERR_INVALID_HOSTNAME);
		return TB_ERR;
	}
	
	h = hostinf->he;

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port   = htons(port);

	while( ++retries <= S->retries ) 
		{
			n = 0;
			while( h->h_addr_list[n] )
				{
					s->sin_addr = *(struct in_addr *)(h->h_addr_list[n++]);
					
					if( (S->sock = socket(PF_INET, myProto, 0)) == -1) {	
						tb_error("tb_initSocket_IP: socket error <%s:%d> %d: %s\n", 
										 S->name, S->port, errno, strerror(errno));
						S->status = TB_UNSET ; 
						hostinf_free(hostinf);
						set_tb_errno(TB_ERR_SOCKET_FAILED);
						return TB_ERR; 
					}

					set_nonblock_flag(S->sock, 1);

					if((connect(S->sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr))) == -1) 
						{ 
							int sv_errno = errno;
							tb_notice("tb_initSocket_IP: connect %d: %s\n", sv_errno, strerror(sv_errno));
 
							if(sv_errno == EINPROGRESS || 
								 sv_errno == EWOULDBLOCK ||
								 sv_errno == EALREADY    ||
								 sv_errno == EISCONN ) 
								{

									if( myProto == TB_UDP ) {
										S->status = TB_CONNECTED;
										hostinf_free(hostinf);
										return TB_OK;
									}

									FD_ZERO(& (S->set));
									FD_SET(S->sock, &(S->set));
									timeout.tv_sec  = S->to_sec;
									timeout.tv_usec = S->to_usec;
									rc = select(S->sock +1, NULL, &(S->set), NULL, &timeout);
									switch( rc ) {
									case -1:
										S->status = TB_BROKEN;
										tb_warn("tb_initSocket_IP: #%d/%d select: %d: %s\n", 
														retries, S->retries,
														errno, strerror(errno));
										close(S->sock);
										continue;  // may be is there another ip for this address name

									case 0: //timeout
										S->status = TB_TIMEDOUT ;
										tb_warn("tb_initSocket_IP: #%d/%d <%s:%d:%s> connect timeout\n", 
														retries, S->retries,
														S->name, S->port, inet_ntoa(s->sin_addr));
										close(S->sock);
										continue;

									default:
										if( FD_ISSET(S->sock, &(S->set)) ){
											rc = sizeof(ret);
											getsockopt(S->sock, SOL_SOCKET, SO_ERROR, (void *)&ret, &rc);
											if(ret == 0) {
												set_nonblock_flag(S->sock, 0);
												S->status = TB_CONNECTED;
												hostinf_free(hostinf);
												// we are now connected
#ifdef WITH_SSL
												if(S->ssl) {
													return tb_connectSSL(O);
												}
#endif
												return TB_OK;
											}
										}
										close(S->sock);
										continue;

									}
								} 
							else 
								{
									tb_warn("tb_initSocket_IP: #%d/%d <%s:%d:%s> %d: %s\n", 
													retries, S->retries,
													S->name, S->port, 
													inet_ntoa(s->sin_addr), 
													sv_errno, strerror(sv_errno)); 
									close(S->sock);
									continue;
								}
						} 
					else 
						{
							set_nonblock_flag(S->sock, 0);
							S->status = TB_CONNECTED;
							hostinf_free(hostinf);
#ifdef WITH_SSL
							if(S->ssl) {
								return tb_connectSSL(O);
							}
#endif
							return TB_OK;
						}
				}
			if(retries < S->retries) {
				tb_warn("tb_initSocket_IP: <%s:%d failed %d/%d (next try in %d s)\n", 
								S->name, S->port,
								retries, S->retries, S->retry_lap);
				sleep(S->retry_lap);
			}
    }

	tb_error("tb_initSocket_IP: <%s> can't connect \n", S->name, S->port);
	hostinf_free(hostinf);
	set_tb_errno(TB_ERR_CONNECT_FAILED);	

	return TB_ERR;
}







Socket_t tb_close_sock(Socket_t S) {
	if(tb_valid(S, TB_SOCKET, __FUNCTION__) && tb_getSockFD(S) != -1) {
		sock_members_t m = XSock(S);
		close(m->sock);
		m->status = TB_DISCONNECTED;
		m->sock   = -1;
	}
	return S;
}


Socket_t tb_sock_clear(Socket_t O) {
	if(tb_valid(O, TB_SOCKET, __FUNCTION__)) {
		sock_members_t m = XSock(O);
		if(m->sock != -1) {
			close(m->sock);
		}
#ifdef WITH_SSL
		if(m->ssl)   {
			if(XSsl(O)->cx ) {
				SSL_shutdown(XSsl(O)->cx);
			}
		}
#endif
		if(m->buffer) {
			tb_Free(m->buffer);
			m->buffer = 0;
		}

		m->status = TB_UNSET;
		m->sock   = -1;
		//obsolete		members->px_seq = 0;
	} else {
		return NULL;
	}
	return O;
}


void *tb_sock_free(Socket_t O) {
	if(tb_valid(O, TB_SOCKET, __FUNCTION__)) {
		sock_members_t m = XSock(O);

		if(tb_getSockFD(O) != -1) close(m->sock);
		if(m->addr_family == TB_UNIX && (m->name != NULL) ) {
			unlink(m->name);
		}
		if(m->name) {
			tb_xfree(m->name);
		}
		if(m->buffer) {
			tb_Free(m->buffer);
		}

		if(m->server)   {
			sock_server_t sm = XServer(O);
			if(sm->nb_t_mtx) tb_xfree(sm->nb_t_mtx);
			// fixme: cloning a server won't clone acl ?
			if( sm->acl ) { //only *real* servers owns acl
				_free_srv_acl(XAcl(O));
			} 
			tb_xfree(m->server);
		}
#ifdef WITH_SSL
		if(m->ssl)   {
			sock_ssl_t ssl =XSsl(O);
			if(ssl->cx ) {
				SSL_shutdown(ssl->cx);
				SSL_free(ssl->cx);
			}
			if(ssl->session) SSL_SESSION_free(ssl->session);
			if(ssl->ctx) SSL_CTX_free(ssl->ctx);
			tb_xfree(m->ssl);
		}
#endif
		O->isA = TB_SOCKET;
		tb_freeMembers(O);
		return tb_getParentMethod(O, OM_FREE);
	}
	return NULL;
}


void tb_sock_ip_dump(Socket_t O, int level) {
  int i;
	socklen_t len = sizeof(struct sockaddr);
	struct sockaddr_in sa;
	char *sock_status[] ={ "UNSET", "DISCONNECTED", "CONNECTED", "LISTENING", "TIMEOUT" };
	sock_members_t m = XSock(O);

  for(i = 0; i<level; i++) fprintf(stderr, " ");
	if(m->status == TB_CONNECTED) {
		getpeername(m->sock, (struct sockaddr *)&sa, &len); 
		fprintf(stderr, "<%p:%d> SOCKET IP [PROTO: %d FD:%d] status: %s <PEER: %s:%d>\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status],
						(char *)inet_ntoa(sa.sin_addr),
						sa.sin_port
					);
	} else if(m->status == TB_LISTENING) {
		fprintf(stderr, "<%p:%d> SOCKET IP SERVER [PROTO: %d FD:%d] status: %s callback: %p\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status],
						m->server->callback
					);
	}	else {
		fprintf(stderr, "<%p:%d> SOCKET IP [PROTO: %d FD:%d] status: %s\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status]
					);
	}
}

void tb_sock_unix_dump(Socket_t O, int level) {
  int i;
	socklen_t len = sizeof(struct sockaddr);
	struct sockaddr_in sa;
	char *sock_status[] ={ "UNSET", "DISCONNECTED", "CONNECTED", "LISTENING", "TIMEOUT" };
	sock_members_t m = XSock(O);

  for(i = 0; i<level; i++) fprintf(stderr, " ");
	if(m->status == TB_CONNECTED) {
		getpeername(m->sock, (struct sockaddr *)&sa, &len); 
		fprintf(stderr, "<%p:%d> SOCKET UNIX [PROTO: %d FD:%d] status: %s <PEER: %s:%d>\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status],
						(char *)inet_ntoa(sa.sin_addr),
						sa.sin_port
					);
	} else if(m->status == TB_LISTENING) {
		fprintf(stderr, "<%p:%d> SOCKET UNIX SERVER [PROTO: %d FD:%d] status: %s callback: %p\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status],
						m->server->callback
					);
	}	else {
		fprintf(stderr, "<%p:%d> SOCKET UNIX [PROTO: %d FD:%d] status: %s\n",
						O, O->refcnt, 
						m->proto, 
						m->sock, 
						sock_status[m->status]
					);
	}
}



int set_nonblock_flag (int fd, int value) {
  int oldflags = fcntl(fd, F_GETFL, 0);
	int rc;
  if (oldflags == -1) {
		tb_crit("fcntl: failed to get flags from fd %d\n", fd);
		tb_crit("fcntl: ernno %d (%s)\n", errno, strerror(errno));
		set_tb_errno(TB_ERR_FCNTL_FAILED);
		return TB_ERR;
	}

  if (value != 0) {
		oldflags |=  O_NONBLOCK;
	} else {
		oldflags &= ~O_NONBLOCK;
	}

	if((rc = fcntl(fd, F_SETFL, oldflags)) == -1) {
		tb_crit("fcntl: failed to set NON_BLOCK flag to %d (fd %d)\n", value, fd);
		tb_crit("fcntl: ernno %d (%s)\n", errno, strerror(errno));
		set_tb_errno(TB_ERR_FCNTL_FAILED);
	}
	return rc;
}


/** Toggle Socket_t's TCP_NDELAY OFF.
 * \ingroup Socket
 * Don't use if you don't know what it's for.
 *
 * \warning only makes sense over \em tcp/ip Socket_t
 * @return RETCODE
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * - TB_ERR_INVALID_PROTOCOL : So's protocol is not TCP/IP
 * - TB_ERR_UNSET : socket is unset
 * - TB_ERR_BAD : setsockopt failed
 * @see tb_Socket
 */
retcode_t tb_setSockNoDelay(Socket_t S) {
	if(tb_valid(S, TB_SOCKET, __FUNCTION__)) {
		int flag = 1;
		int res;
		sock_members_t m = XSock(S);

		if(! m->proto && TB_TCP) {
			tb_warn("tb_setSockNoDelay : wrong proto for TCP_NODELAY\n");
			set_tb_errno(TB_ERR_INVALID_PROTOCOL);
			return TB_KO;
		}
		if(! m->sock > 0) {
			tb_warn("tb_setSockNoDelay[%s:%d] : setup sock first\n", 
							m->name, m->port);
			set_tb_errno(TB_ERR_UNSET);
			return TB_KO;
		}

		res = setsockopt(m->sock, 
										 IPPROTO_TCP, 
										 TCP_NODELAY,
										 (char *)&flag, sizeof(int));

		if(res < 0) {
			tb_error("tb_setSockNoDelay : %s\n", strerror(errno));
			set_tb_errno(TB_ERR_BAD);
			return TB_ERR;
		}

		return TB_OK;
	}
	return TB_ERR;
}



/** Set timeout values for Socket_t.
 * \ingroup Socket
 * Set timeout for various select(2) calls performed on Socket_t. 
 * See select(2) for details about timeval structure.
 * @param S target Socket_t
 * @param sec number of seconds
 * @param usec number of micro-seconds
 * @return retcode_t
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * @see tb_Socket, tb_getSockTO
 */
retcode_t tb_setSockTO(Socket_t S, long int sec, long int usec) {
	if(tb_valid(S, TB_SOCKET, __FUNCTION__)) {
		sock_members_t m = XSock(S);
		m->to_sec = sec;
		m->to_usec = usec;

		return TB_OK;
	}
	return TB_ERR;
}

/** Get timeout values for Socket_t.
 * \ingroup Socket
 * Get timeout for various select(2) calls performed on Socket_t. 
 * See select(2) for details about timeval structure.
 * @param S target Socket_t
 * @param sec will return number of seconds
 * @param usec will return number of micro-seconds
 * @return retcode_t
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * @see tb_Socket, tb_setSockTO
 */
retcode_t tb_getSockTO(Socket_t S, long int *sec, long int *usec) {
	if(tb_valid(S, TB_SOCKET, __FUNCTION__)) {
		sock_members_t m = XSock(S);
		*sec =  m->to_sec;
		*usec = m->to_usec;
		return TB_OK;
	}
	return TB_ERR;
}

/** Get status  for Socket_t.
 * \ingroup Socket
 * @param S target Socket_t
 * @return TB_SOCKSTATUS or TB_ERR on error
 * Socket_t internal status is of type TB_SOCKSTATUS:
 * - TB_BROKEN
 * - TB_UNSET
 * - TB_DISCONNECTED
 * - TB_CONNECTED
 * - TB_LISTENING
 * - TB_TIMEDOUT
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * @see tb_Socket, tb_setSockTO
 */
int tb_getSockStatus(Socket_t S) {
	if(tb_valid(S, TB_SOCKET, __FUNCTION__)) {
		return XSock(S)->status;
	}
	return TB_ERR;
}

Socket_t tb_sock_clone(Socket_t So) {
	if(tb_valid(So, TB_SOCKET, __FUNCTION__)) {
		Socket_t new; 
		sock_members_t N, S; 

		S = XSock(So);
		
		new = tb_Socket(S->addr_family | S->proto, 
										S->name,
										S->port);

		N = XSock(new);
		N->sock        = S->sock;
		N->proto       = S->proto;
		N->addr_family = S->addr_family;

		N->port = S->port;
		N->status = S->status;
		N->to_sec = S->to_sec;
		N->to_usec = S->to_usec;
		N->retries = S->retries;
		N->retry_lap = S->retry_lap;

		if(S->server) {
			N->server = (sock_server_t) tb_xcalloc(1, sizeof(struct sock_server));
			N->server->max_threads = S->server->max_threads;
			N->server->callback = S->server->callback;
			N->server->args = S->server->args;
		}
#ifdef WITH_SSL
		if(S->ssl) {
			N->ssl = tb_xcalloc(1, sizeof(struct sock_ssl));
			XSsl(new)->ssl_method = XSsl(So)->ssl_method;
			if(XSsl(So)->CA_path) XSsl(new)->CA_path = tb_xstrdup(XSsl(So)->CA_path);
			if(XSsl(So)->CA_file) XSsl(new)->CA_file = tb_xstrdup(XSsl(So)->CA_file);
			if(XSsl(So)->cert) XSsl(new)->cert       = tb_xstrdup(XSsl(So)->cert);
			if(XSsl(So)->pwd) XSsl(new)->pwd         = tb_xstrdup(XSsl(So)->pwd);
			if(XSsl(So)->cipher) XSsl(new)->cipher   = tb_xstrdup(XSsl(So)->cipher);
		}
#endif

		return new;
	}
	return NULL;
}


void hostinf_free(struct hostinf *h) {
	if(! h)      return;
	if(h->he)    tb_xfree(h->he);
	if(h->data)  tb_xfree(h->data);
	tb_xfree(h);
}

struct hostinf * gethost_sin_addr(char *host) {
	int res;
	struct hostinf *retval = tb_xcalloc(1, sizeof(struct hostinf));
#ifdef AIX
	struct hostent_data *host_data = tb_xcalloc(1, sizeof(struct hostent_data));
	struct hostent *hp = tb_xcalloc(1, sizeof(struct hostent));

	retval->data = host_data;
	retval->he   = hp;

#elif defined LINUX
	struct in_addr inaddr;
	struct hostent *hp;
	struct hostent *hostbuf = tb_xcalloc(1, sizeof(struct hostent));
	int herr;
	size_t hstbuflen;
	char *tmphstbuf;
	int maxtries = 2;
	hstbuflen = 1024;
	tmphstbuf = tb_xmalloc (hstbuflen);

	retval->data = tmphstbuf;

#else
	return NULL;
#endif

	
#ifdef LINUX		 
 do_it_again:

	if(inet_aton(host, &inaddr) != 0) { // IPv4 dotted address

		while ((res = gethostbyaddr_r ((char *)&inaddr, sizeof(struct in_addr), AF_INET,
																	 hostbuf, tmphstbuf, hstbuflen,
																	 &hp, &herr)) == ERANGE) {
			/* Enlarge the buffer.  */
			hstbuflen *= 2;
			tmphstbuf = tb_xrealloc (tmphstbuf, hstbuflen);
		}

	} else {

		while ((res = gethostbyname_r (host, hostbuf, tmphstbuf, hstbuflen,
																	 &hp, &herr)) == ERANGE) {
			/* Enlarge the buffer.  */
			hstbuflen *= 2;
			tmphstbuf = tb_xrealloc (tmphstbuf, hstbuflen);
		}
	}

	if(res) {
		tb_warn("gethost_sin_addr(%s): %d (%s)\n", host, herr, strerror(herr));
		if(herr == EINTR && (maxtries--) >0) goto do_it_again;
		if( retval) tb_xfree(retval);
		if(hostbuf) tb_xfree(hostbuf);
		if( tmphstbuf) tb_xfree(tmphstbuf);
		return NULL;
	}
	retval->he = hp;
#elif defined AIX
	if((res = gethostbyname_r(host, hp, host_data)) != 0) {
		tb_xfree(hp);
		tb_xfree(host_data);
		tb_xfree(retval);
		return NULL;
	}
#else
		return NULL;
#endif	
	
	if (res || hp == NULL) {
		tb_xfree(retval->data);
		tb_xfree(retval);
		return NULL;
	}

	return retval;
}


