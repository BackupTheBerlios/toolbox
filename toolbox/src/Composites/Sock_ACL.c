//------------------------------------------------------------------
//$Id: Sock_ACL.c,v 1.1 2004/05/12 22:04:49 plg Exp $
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



#include <stdio.h>
#include <time.h>
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

#include "Toolbox.h"
#include "bplus_tree.h"
#include "Socket.h"
#include "Sock_ACL.h"



/* TODO: move all this fine stuff to ACL interface */

static inline void * _P2p(tb_Object_t O) { return (O)? P2p(O) : NULL ; }

int rstrcmp(char *s1, char *s2);
void acl_dump_chain(bptree_t Btree);


int check_sock_acl(srv_acl_t acl, struct sockaddr *incoming) {
	time_t now;
	char * name = (char *)inet_ntoa(((struct sockaddr_in *)(incoming))->sin_addr);

	tb_info("check DENY acl rules for <%s>...\n", name);
	if( acl_host_match( acl->deny, name)) {
			//	if( acl_host_match( acl->DENY, incoming)) {
		tb_info("check ALLOW acl rules for <%s>...\n", name);
		if(! acl_host_match( acl->allow, name)) {

		//		if(! acl_host_match( acl->ALLOW, incoming)) {
			tb_info("ACL: not in ALLOW \n");
			return ACL_REJECT;
		}
	} else {
		tb_info("ACL: not in DENY \n");
	}

	// first check global rules
	now = time(NULL);

	if( acl->max_global_hps ) {
		tb_warn("check_sock_acl: checking global hps tm=%d now=%d hps=%d max=%d\n",
						acl->global_freq_acl->timestamp, now, acl->global_freq_acl->nb_hps,
						acl->max_global_hps);

		if(acl->global_freq_acl->timestamp == now) {
			if(acl->global_freq_acl->nb_hps++ >= acl->max_global_hps) {
				tb_warn("check_sock_acl: too many hits per seconds (global check) (%d)\n",
								acl->max_global_hps );
				return ACL_REJECT;
			}
		} else {
			acl->global_freq_acl->timestamp = now;
			acl->global_freq_acl->nb_hps=1;
		}
	}

	if( acl->max_global_simult ) {
		tb_warn("check_sock_acl: checking global simult (%d/%d)\n", acl->max_global_simult);

		if(acl->global_freq_acl->nb_simult++ >= acl->max_global_simult) {
			tb_warn("check_sock_acl: too many simltanous hits (global check) (%d)\n",
							acl->max_global_simult );
			return ACL_REJECT;
		}
	}

	// now check per host acl

	if( acl->max_host_hps ) {
		freq_acl_t F = _P2p(tb_Get(acl->host_freq_acl, name));

		tb_warn("check_sock_acl: checking host hps (%d)\n",acl->max_host_hps);
		if( F != NULL ) {
			tb_warn("check_sock_acl: checking host hps (%d/%d)\n",
							F->nb_hps,
							acl->max_host_hps);
			if(F->timestamp == now) {
				if(F->nb_hps++ >= acl->max_host_hps) {
					tb_warn("check_sock_acl: [%s] too many hits per seconds (by host check) (%d)\n",
									name, acl->max_global_hps );
					return ACL_REJECT;
				}
			} else {
				F->timestamp = now;
				F->nb_hps=1;
			}
		} else {
			F = tb_xcalloc(1, sizeof(struct freq_acl));
			F->timestamp = now;
			F->nb_hps = 1;
			F->nb_simult = 1;
			tb_Replace(acl->host_freq_acl, tb_Pointer(F, tb_xfree), name);
		}
	}

	if( acl->max_host_simult ) {
		freq_acl_t F = _P2p(tb_Get(acl->host_freq_acl, name));
		tb_warn("check_sock_acl: checking host simult (%d)\n",acl->max_host_simult);
		if( F != NULL ) {
			F->nb_simult++;
			tb_warn("check_sock_acl: checking host<%s> simult  (%d/%d)\n", name,
							F->nb_simult,
							acl->max_host_simult);
			if(F->nb_simult >= acl->max_host_simult) {
				tb_warn("check_sock_acl: [%s] too many simultanous hits(by host check) (%d)\n",
								name, acl->max_global_hps );
				return ACL_REJECT;
			}
		} else {
			F = tb_xcalloc(1, sizeof(struct freq_acl));
			F->timestamp = now;
			F->nb_hps = 1;
			F->nb_simult = 1;
			tb_Replace(acl->host_freq_acl, tb_Pointer(F, tb_xfree), name);
		}
	}

	return ACL_ALLOW;
}


int acl_host_match(Dict_t D, char *incoming) {
	if(tb_Exists(D, "NONE")) {
		tb_info("acl_host_match: found NONE");
		return 0;
	} else if(tb_Exists(D, "ALL")) {
		tb_info("acl_host_match: found ALL");
		return 1;
	} else if(tb_Exists(D, incoming)) {
		tb_info("acl_host_match: <%s>", incoming);
		return 1;
	} else { 
		struct hostinf *hostinf;
		struct hostent *h;
		int i, j;
		if(( hostinf = gethost_sin_addr(incoming)) != NULL) {
			bpt_node_t n = XBpt(D)->first;
			h = hostinf->he;

			while(n != NULL) {
				for(i = 0; i<n->cnt; i++) {
					if( rstrcmp(n->keys[i].key, h->h_name) == 0) {
						tb_info("acl_host_match: [%s] match<%s> ",(char *)n->keys[i].key,
											 h->h_name);
						return 1;
					} else {
						tb_info("acl_host_match: [%s] not matching <%s> ",(char *)n->keys[i].key,
										h->h_name);
					}
				}
				n = n->next;
			}

			for(j=0; h->h_aliases[j] != NULL; j++) {
				while(n != NULL) {
					n = XBpt(D)->first;
					for(i = 0; i<n->cnt-1; i++) {
						tb_info("acl_host_match: try <%s> (alias)", h->h_aliases[j]);
						if( rstrcmp((char *)n->keys[i].key, h->h_aliases[j]) == 0) {
							tb_info("acl_host_match: [%s] match<%s> ",(char *)n->keys[i].key,
											h->h_aliases[j]);
							return 1;
						} else {
							tb_info("acl_host_match: [%s] not matching <%s> ",(char *)n->keys[i].key,
											h->h_aliases[j]);
						}
					}
					n = n->next;
				}
			}
		} else {
			tb_info("host not found <%s>\n",incoming);
		} 
		hostinf_free(hostinf);
	}
	return 0;
}


srv_acl_t _new_sock_acl() {

	srv_acl_t acl = tb_xcalloc(1, sizeof(struct srv_acl));
	acl->deny  = tb_Dict(KT_STRING, 0);
	acl->allow = tb_Dict(KT_STRING, 0);
	acl->host_freq_acl = tb_Hash();
	acl->global_freq_acl = tb_xcalloc(1, sizeof(struct freq_acl));
	
	return acl;
}


int _free_srv_acl(srv_acl_t acl) {
	tb_Free(acl->deny);
	tb_Free(acl->allow);
	tb_Free(acl->host_freq_acl);
	tb_xfree(acl->global_freq_acl);
	// fixme: some bits still allocated
	tb_xfree(acl);
	return 1;
}

// is .abc.net match toto.abc.net
// rstrcmp("toto.abc.net", ".abc.net")
int rstrcmp(char *s1, char *s2) {
	int len1 = strlen(s1);
	int len2 = strlen(s2);
	int cmp = -1;
	int i, len = TB_MIN(len1, len2);
	char *a = s1+(len1 -1);
	char *b = s2+(len2 -1);
	for(i=0; i<len; i++) {
		cmp = (unsigned char)*(a-i) - (unsigned char)*(b-i);
		if(cmp != 0) {
			return cmp;
		}
	}
	return cmp;
}


// public

int tb_sockACL(Socket_t S, int use_acl) {
	//fixme: need std validation
	pthread_mutex_lock(&(XAcl(S)->mtx));
	XServer(S)->acl->use_acl = (use_acl) ? 1 : 0;
	pthread_mutex_unlock(&(XAcl(S)->mtx));
	return 1;
}

int tb_sockACL_ADD(Socket_t S, acl_list_t list, char *pattern) {
	//fixme: need std validation
	int rc = TB_ERR;
	pthread_mutex_lock(&(XAcl(S)->mtx));
	switch( list ) {
	case ACL_DENY:
		rc = tb_Insert(XServer(S)->acl->deny, tb_String(NULL), pattern);
		break;
	case ACL_ALLOW:
		rc = tb_Insert(XServer(S)->acl->allow, tb_String(NULL), pattern);
		break;
	}
	pthread_mutex_unlock(&(XAcl(S)->mtx));
	
	return rc;
}

int tb_sockACL_DEL(Socket_t S, acl_list_t list, char *pattern) {
	//fixme: need std validation
	int rc = TB_ERR;
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	switch( list ) {
	case ACL_DENY:
		rc = tb_Remove(XServer(S)->acl->deny, pattern);
		break;
	case ACL_ALLOW:
		rc = tb_Remove(XServer(S)->acl->allow, pattern);
		break;
	}
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));

	return rc;
}

int tb_sockACL_CLEAR(Socket_t S, acl_list_t list) {
	//fixme: need std validation
	int rc = TB_ERR;
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	switch( list ) {
	case ACL_DENY:
		tb_Clear(XServer(S)->acl->deny);
		rc = 1;
		break;
	case ACL_ALLOW:
		tb_Clear(XServer(S)->acl->allow);
		rc = 1;
		break;
	}
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));
	return rc;
}
	
Vector_t tb_sockACL_LIST(Socket_t S, acl_list_t list) {
	//fixme: need std validation
	Vector_t V = tb_Vector();
	bptree_t  Btree;
	bpt_node_t n;

	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	switch( list ) {
	case ACL_DENY:
		Btree = XBpt(XServer(S)->acl->deny);
		break;
	case ACL_ALLOW:
		Btree = XBpt(XServer(S)->acl->allow);
		break;
	default:
		tb_Free(V);
		pthread_mutex_unlock(&(XServer(S)->acl->mtx));
		return NULL;
	}
	n = Btree->first;

	while(n != NULL) {
		int i;
		for(i = 0; i<n->cnt; i++) {
			tb_Push(V, tb_String("%s", (char *)n->keys[i].key));
		}
		n = n->next;
	}
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));	

	return V;
}


int tb_sockACL_set_global_max_hps(Socket_t S, int max_hps) {
	//fixme: need std validation
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	XServer(S)->acl->max_global_hps = max_hps;
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));
	return 1;
}

int tb_sockACL_get_global_max_hps(Socket_t S) {
	//fixme: need std validation
	return XServer(S)->acl->max_global_hps;
}

int tb_sockACL_set_global_max_simult(Socket_t S, int max_simult) {
	//fixme: need std validation
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	XServer(S)->acl->max_global_simult = max_simult;
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));
	return 1;
}

int tb_sockACL_get_global_max_simult(Socket_t S) {
	//fixme: need std validation
	return XServer(S)->acl->max_global_simult;
}

int tb_sockACL_set_host_max_hps(Socket_t S, int max_hps) {
	//fixme: need std validation
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	XServer(S)->acl->max_host_hps = max_hps;
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));
	return 1;
}

int tb_sockACL_get_host_max_hps(Socket_t S) {
	//fixme: need std validation
	return XServer(S)->acl->max_host_hps;
}

int tb_sockACL_set_host_max_simult(Socket_t S, int max_simult) {
	//fixme: need std validation
	pthread_mutex_lock(&(XServer(S)->acl->mtx));
	XServer(S)->acl->max_host_simult = max_simult;
	pthread_mutex_unlock(&(XServer(S)->acl->mtx));
	return 1;
}

int tb_sockACL_get_host_max_simult(Socket_t S) {
	//fixme: need std validation
	return XServer(S)->acl->max_host_simult;
}



void acl_dump_chain(bptree_t Btree) {
	int i;
	bpt_node_t n = Btree->first;
	while(n != NULL) {
		//		fprintf(stderr, "%p     : %p <-", n, n->prev);
		for(i = 0; i<n->cnt; i++) {
			fprintf(stderr, "%s/", (char *)n->keys[i].key);
		}
		n = n->next;
	}
	fprintf(stderr, "\n");
}

void tb_dumpACL(Socket_t S) {
	fprintf(stderr, "DENY: ");
	acl_dump_chain(XBpt(XAcl(S)->deny));
	fprintf(stderr, "\nALLOW: ");
	acl_dump_chain(XBpt(XAcl(S)->allow));
	fprintf(stderr, "\n");
}
