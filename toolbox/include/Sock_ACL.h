//------------------------------------------------------------------
// $Id: Sock_ACL.h,v 1.1 2004/05/12 22:04:48 plg Exp $
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


#ifndef SOCK_ACL_H
#define SOCK_ACL_H

#include <pthread.h>
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

#include "Toolbox.h"
#include "Socket.h"

#define ACL_REJECT 0
#define ACL_ALLOW  1


struct freq_acl {
	time_t timestamp;
	int    nb_hps;
	int    nb_simult;
};
typedef struct freq_acl *freq_acl_t;


struct srv_acl {
	int             use_acl;
	pthread_mutex_t mtx;
	Hash_t          allow;
	Hash_t          deny;

	int             max_global_hps;
	int             max_host_hps;
	int             max_global_simult;
	int             max_host_simult;

	Hash_t          host_freq_acl;
	freq_acl_t      global_freq_acl;

/* 	int             GLOBAL_MAX_SIMULT; */
/* 	int             FREQ_MAX; */
/* 	int             cur_global_access; */
/* 	time_t          last_access; */
/* 	int             last_access_nb; */
	int (*srv_acl_callback_t)(struct srv_acl *,void *);
};
typedef struct srv_acl *srv_acl_t;
typedef int (*srv_acl_callback_t)(srv_acl_t,void *);

srv_acl_t _new_sock_acl          (void);
int       _free_srv_acl          (srv_acl_t acl);
int       check_sock_acl         (srv_acl_t acl, struct sockaddr *incoming);
//int       acl_host_match         (Dict_t D, struct sockaddr *incoming);
int       acl_host_match         (Dict_t D, char *incoming);
// public interface --> Toolbox.h

//FIXME: (CODEme) int tb_sock_ACL2Xml(Socket_t S, XmlDoc_t acl_xml); 
//FIXME: (CODEme) XmlDoc_t tb_sock_Xml2ACL(Socket_t S);
 



#endif
