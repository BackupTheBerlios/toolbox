//------------------------------------------------------------------
// 	$Id: Socket.h,v 1.2 2004/07/01 21:37:18 plg Exp $
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

#ifndef __TB_SOCKET_H
#define __TB_SOCKET_H

#include "Toolbox.h"
#include "Objects.h"
#include "Sock_ACL.h"

#define MAX_SRV_NAME      80
#define MAX_REPLY         80      // reply string (see request)

#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#ifdef WITH_SSL
#include <bio.h>
#include <asn1.h>
#include <x509.h>
#include <ssl.h>
#include <err.h>
#include <pem.h>
#endif

#ifdef WITH_XTI
#undef TCP_NODELAY
#undef TCP_MAXSEG 
#undef IP_OPTIONS
#undef IP_TOS
#undef IP_TTL


#include <pthread.h>
#include <xti_xx25/sys/xti.h>
#include <xti_api/xti_ns.h>
#include <xti_xx25/xx25addr.h>
/* Pb with netinet/in.h when xti_xx25/sys/xti.h is included, see configure.in */
#define T_X25_RST       0x2000 

struct xti_extra {
	int             status;
	int             reason;
	char          * host;
	char          * service;
	char          * opts;
	struct xtitp    tp;
  struct t_call * sndcall;
  struct t_bind * bind;
  struct t_info   info;
};
typedef struct xti_extra *xti_extra_t;
#define XXTI(A)      ((xti_extra_t)((sock_extra_t )A->extra)->xti)

struct x25_err {
	int    errcode;
	char * errstring;
};


#endif


struct sock_server {
	int                spawn_method;           // fork or thread
	int                max_threads;             
  pthread_mutex_t  * nb_t_mtx; 
	void             * callback;           
	void             * args;              
	srv_acl_t          acl;

	pthread_mutex_t    shutdown_mtx;
	pthread_cond_t     shutdown_cnd;
	int                size;
};
typedef struct sock_server * sock_server_t;

struct hostinf {
	struct hostent *he;
	void *data;
};





#ifdef WITH_SSL

struct sock_ssl {
	ssl_meth_t ssl_method;       // will be an enum (1->4) = TLSv1/SSLv2/SSLv3/SSLv23
	char     * CA_path;          // path to CA dir, w/ hash symlinks to CA certs
	char     * CA_file;          // full path of CA file
	char     * cert;             // full path of cert file (cert+privkey in PEM format)
	char     * pwd;              // password for private key
	char     * cipher;           // Default is : "ALL:!ADH:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP"

	enum ssl_mode mode;

	int        verify_error;
	int        verify_depth;

	SSL_SESSION * session;
  X509_NAME  * xn;
  SSL        * cx;
  SSL_CIPHER * sc;
  X509       * peer;
	SSL_CTX    * ctx;
	BIO        * bio;
};
typedef struct sock_ssl *sock_ssl_t;

int tb_connectSSL( Socket_t S );
#endif

struct sock_members {
	int                sock;                    // fd socket
	int                addr_family;             // IP / UNIX namespace
	int                proto;                   // type de protocole (UPD/TCP)
	char             * name;                    // enpoint name (host/file)
	unsigned short int port;                    // enpoint port if applicable
	TB_SOCKSTATUS      status;                  // socket state
	fd_set             set;
	int                to_sec;                  // timeout val (sec)
	int                to_usec;                 //             (millisec)
	int                retries;                 // IO op retries
	int                retry_lap;               // wait between retries
	String_t           buffer;                  // for readSockLine operations
	sock_server_t      server;
	//obsolete	unsigned long int  px_seq;                  // packet exchange seq id
#ifdef WITH_SSL
	sock_ssl_t         ssl;
#endif
#ifdef WITH_XTI
	xti_extra_t        xti;
#endif

};
typedef struct sock_members *sock_members_t;




//#define XSOCK(A)     ((sock_extra_t )(A)->extra)
//#define XSERVER(A)   ((sock_server_t)((sock_extra_t )(A)->extra)->server)
//#define XACL(A)      ((srv_acl_t)((sock_server_t)((sock_extra_t )(A)->extra)->server)->acl)


inline sock_members_t  XSock(Socket_t This);
inline sock_server_t   XServer(Socket_t This);
inline srv_acl_t       XAcl(Socket_t This);

#ifdef WITH_SSL
//#define XSSL(A)      ((sock_ssl_t)((sock_extra_t )(A)->extra)->ssl)
inline sock_ssl_t     XSsl(Socket_t This);

void   info_cb       (SSL *s,int where,int ret);
int    verify_cb     (int ok, X509_STORE_CTX *ctx);
int    pass_cb       (char *buf,int len, int verify, void *userdata);
long   bio_dump_cb   (BIO *bio, int cmd, char *argp,
                        int argi, long argl, long ret);
void   ssl_barf_out  (Socket_t S);


#endif

typedef struct {
	pthread_mutex_t  * mutex;
	pthread_cond_t   * cond;
	int              * nb;
	Socket_t           So;
	Socket_t           Parent; // !! dangerous !
	void             * user_args;
} spawn_args;


void       * spawner          (void *arg);
Socket_t     tb_sock_new      (tb_Object_t O, int proto, char *name, int port);
#ifdef WITH_XTI
Socket_t      tb_sock_x25_new (tb_Object_t O, char *host, char *service, char *opts);
int          tb_initSocket_X25(Socket_t O);
int          tb_readSock_X25  (Socket_t S, tb_Object_t Msg, int maxlen);
int          tb_writeSock_X25 (Socket_t S, char *msg);
void       * tb_Accept_x25    (Socket_t O);
RETCODE tb_initSockServer_x25 (Socket_t O, 
															 void *callback, // int(*)(Socket_t)
															 void *cb_args);

const char * str_x25cause     (int cause);
const char * str_x25diag      (int diag);
#endif
void       * tb_sock_free     (Socket_t O);
void         tb_sock_dump     (Socket_t O, int level);
tb_Object_t  tb_sock_clone    (Socket_t O);
Socket_t     tb_sock_clear    (Socket_t S);

void             hostinf_free                (struct hostinf *h);
struct hostinf * gethost_sin_addr            (char *host);



#endif








