//========================================================================
// 	$Id: Sock_ssl.c,v 1.1 2004/05/12 22:04:49 plg Exp $
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
#include "Toolbox.h"
#include "Composites.h"
#include "Memory.h"
#include "Error.h"

#ifdef WITH_SSL
static void __tb_init_SSL_once();

static void __tb_init_SSL_once() {
	SSL_load_error_strings();
	SSL_library_init();
}


/* Debug callback tracks raw SSL buffers.  Initialized by 
 * BIO_set_callback().  Invoked when debug level is >= 4.
 */
long bio_dump_cb(BIO *bio, int cmd, char *argp,
                        int argi, long argl, long ret) {
  BIO *out;
  out=(BIO *)BIO_get_callback_arg(bio);
  if (out == NULL) return(ret);
  if (cmd == (BIO_CB_READ|BIO_CB_RETURN)) {
    BIO_printf(out,"read from %08X [%08lX] (%d bytes => %ld (0x%X))\n",
               bio,argp,argi,ret,ret);
    BIO_dump(out,argp,(int)ret);
    return(ret);
  }
  else if (cmd == (BIO_CB_WRITE|BIO_CB_RETURN)) {
    BIO_printf(out,"write to %08X [%08lX] (%d bytes => %ld (0x%X))\n",
               bio,argp,argi,ret,ret);
    BIO_dump(out,argp,(int)ret);
  }
  return(ret);
}

/* Passphrase callback.  Initialized by SSL_CTX_set_default_passwd_cb.
 * Used by various functions in ssl/ssl_rsa.c.  Just copies passphrase
 * initialized by main() to return buffer.  If you don't specify this
 * and the RSA key is encrypted, the PEM libraries prompt for it at the
 * terminal.
 */


int pass_cb(char *buf, int len, int verify, void *userdata) {
  int i;
	Socket_t So = (Socket_t) userdata;
	if (!TB_VALID(So, TB_SOCKET)) return 0;
  if (XSsl(So)->pwd == NULL) return(0);
  i=strlen(XSsl(So)->pwd);
  i=(i > len)?len:i;
  memcpy(buf, XSsl(So)->pwd, i);
  return(i);
}
/* Debug callback tracks SSL connection states and errors.  
 * Initialized by SSL_CTX_set_info_callback.  Invoked when
 * tb_errorlevel >= TN_NOTICE.  Copied from apps/s_cb.c.
 */
void info_cb(SSL *s,int where,int ret) {
  char *str="undefined";
  int w=where& ~SSL_ST_MASK;
  if (w & SSL_ST_CONNECT) str="SSL_connect";
  else if (w & SSL_ST_ACCEPT) str="SSL_accept";
  if (where & SSL_CB_LOOP) 
    fprintf(stderr,"%s:%s\n",str,SSL_state_string_long(s));
  else if (where & SSL_CB_ALERT) {
    str=(where & SSL_CB_READ)?"read":"write";
    fprintf(stderr,"SSL3 alert %s:%s:%s\n",
            str,
            SSL_alert_type_string_long(ret),
            SSL_alert_desc_string_long(ret));
  }
  else if (where & SSL_CB_EXIT) {
    if (ret == 0)
      fprintf(stderr,"%s:failed in %s\n",str,SSL_state_string_long(s));
    else if (ret < 0) 
      fprintf(stderr,"%s:error in %s\n",str,SSL_state_string_long(s));
  }
}

/* Verify certificate callback.  Gets invoked by crypto/x509/x509_vfy.c:
 * internal_verify() after it checks the signature and the time of 
 * the certificate.  Copied from apps/s_cb.c.  Configured through 
 * verify_depth and verify_error, initialized in main().  Doesn't
 * really do much other than print verify errors to the screen and
 * ignore errors down to verify_depth.  Mainly an example of how
 * OpenSSL let's your app have a say in certificate validating.
 * At this point OpenSSL has checked the signature and dates are 
 * good, so you don't need to do that.  One thing you could do 
 * here would be to verify the certificate status with an on-line
 * service, e.g. via OCSP.
 */
int verify_cb(int ok, X509_STORE_CTX *ctx) {
  char buf[256];
  X509 *err_cert;
  int err,depth;
  BIO* bio_err;
	int verify_error = X509_V_OK, verify_depth = 0;

  if ((bio_err=BIO_new(BIO_s_file())) == NULL) return(0);

  BIO_set_fp(bio_err,stderr,BIO_NOCLOSE);

  err_cert=X509_STORE_CTX_get_current_cert(ctx);
  err=X509_STORE_CTX_get_error(ctx);
  depth=X509_STORE_CTX_get_error_depth(ctx);

  X509_NAME_oneline(X509_get_subject_name(err_cert),buf,256);
  BIO_printf(bio_err,"depth=%d %s\n",depth,buf);
  if (!ok) {
    BIO_printf(bio_err,"verify error:num=%d:%s\n",err,
               X509_verify_cert_error_string(err));
    if (depth > verify_depth) {
      verify_error=X509_V_ERR_CERT_CHAIN_TOO_LONG;
    } else {
      ok=1;
      verify_error=X509_V_OK;
    }
  }
  switch (ctx->error) {
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),buf,256);
    BIO_printf(bio_err,"issuer= %s\n",buf);
    break;
  case X509_V_ERR_CERT_NOT_YET_VALID:
  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    BIO_printf(bio_err,"notBefore=");
    ASN1_TIME_print(bio_err,X509_get_notBefore(ctx->current_cert));
    BIO_printf(bio_err,"\n");
    break;
  case X509_V_ERR_CERT_HAS_EXPIRED:
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    BIO_printf(bio_err,"notAfter=");
    ASN1_TIME_print(bio_err,X509_get_notAfter(ctx->current_cert));
    BIO_printf(bio_err,"\n");
    break;
  }
  BIO_printf(bio_err,"verify return:%d\n",ok);
  BIO_free(bio_err);
  return(ok);
}


retcode_t tb_initSSL(Socket_t    S, 
										 enum ssl_mode  mode,       // SSL_CLIENT | SSL_SERVER
										 ssl_meth_t  method,     // SSL1 | SSL2 | SSL3 | TLS1
										 char      * CA_path,
										 char      * CA_file,
										 char      * cert,
										 char      * pwd,
										 char      * cipher) {
	SSL_METHOD * meth;
	sock_ssl_t m;

	tb_info("tb_initSSL in\n");

	if(!TB_VALID(S, TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(XSock(S)->ssl != NULL ) {
		tb_warn("tb_initSSL: Socket_t allready SSL initialized\n");
		set_tb_errno(TB_ERR_ALLREADY);
		return TB_ERR;
	}

	m = tb_xcalloc(1, sizeof(struct sock_ssl));
	XSock(S)->ssl = m;
	m->ssl_method = method;

	m->mode = method;


	if( CA_path ) m->CA_path = tb_xstrdup(CA_path);
	if( CA_file ) m->CA_file = tb_xstrdup(CA_file);
	if( cert ) 		m->cert    = tb_xstrdup(cert);
	if( pwd ) 		m->pwd     = tb_xstrdup(pwd);		
	if( cipher ) 	m->cipher  = tb_xstrdup(cipher);
	

	__tb_init_SSL_once();

	switch (m->ssl_method) {
  case 1:
		meth = (mode == SSL_CLIENT) ? SSLv23_client_method() : SSLv23_server_method();
    break;
  case 2:
		meth = (mode == SSL_CLIENT) ? SSLv2_client_method() : SSLv2_server_method();
    break;
  case 3:
		meth = (mode == SSL_CLIENT) ? SSLv3_client_method() : SSLv3_server_method();
    break;
  case 4:
		meth = (mode == SSL_CLIENT) ? TLSv1_client_method() : TLSv1_server_method();
    break;
	default:
		meth = NULL;
		goto err;
  }


	if (!(m->ctx = SSL_CTX_new(meth))) {
    tb_warn("tb_initSSL: Cannot create new SSL context\n");
    ERR_print_errors_fp(stderr);
		XSock(S)->status = TB_BROKEN;
		return TB_ERR; 
  }

	if(tb_errorlevel == TB_DEBUG) SSL_CTX_set_info_callback(m->ctx,info_cb);

  if(m->pwd) {
		SSL_CTX_set_default_passwd_cb(m->ctx, pass_cb);
		SSL_CTX_set_default_passwd_cb_userdata(m->ctx, S);
	}
	

	if(m->cert ) {
		if(SSL_CTX_use_certificate_file(m->ctx, m->cert, SSL_FILETYPE_PEM) <= 0) {
			ERR_print_errors_fp(stderr);
			goto err;
		}
		if (SSL_CTX_use_PrivateKey_file(m->ctx, m->cert, SSL_FILETYPE_PEM) <= 0) {
			tb_error("tb_initSSL: Unable to get private key from '%s'\n", 
							 m->cert);
			ERR_print_errors_fp(stderr);
			goto err;
		}
		tb_info("privkey loaded\n");		
		if (!SSL_CTX_check_private_key(m->ctx)) {
			tb_error("tb_initSSL: Private key does not match the certificate public key\n");
			goto err;
		}
		tb_info("tb_initSSL: privkey validated\n");
		tb_info("tb_initSSL: certificate loaded\n");
	}

	if(mode == SSL_CLIENT) {
		SSL_CTX_set_session_cache_mode(m->ctx, SSL_SESS_CACHE_CLIENT);
	} else {
		SSL_CTX_set_session_cache_mode(m->ctx, SSL_SESS_CACHE_SERVER);
		SSL_CTX_set_session_id_context(m->ctx,  "try this one", 12);
	}

  if(m->CA_file || m->CA_path) {
		tb_info("tb_initSSL: loading CAs ...\n");
    if(!SSL_CTX_load_verify_locations(m->ctx, m->CA_file, m->CA_path)) {
			XSock(S)->status = TB_BROKEN;
      tb_warn("tb_initSSL: Cannot load verify locations %s and %s\n",
              m->CA_file, m->CA_path);
      ERR_print_errors_fp(stderr);
			goto err;
    }
		tb_info("tb_initSSL: CA  <%s/%s> loaded\n", m->CA_path, m->CA_file);
		SSL_CTX_set_verify(m->ctx, SSL_VERIFY_PEER, verify_cb);
		SSL_CTX_set_default_verify_paths(m->ctx);
  }

 /* Create and configure SSL connection. */

  if (!(m->cx = (SSL *)SSL_new(m->ctx))) {
    tb_warn("tb_initSSL: Cannot create new SSL context\n");
    ERR_print_errors_fp(stderr);
		goto err;
  }
	tb_info("tb_initSSL: ssl ctx initialized\n");

  /* Use OpenSSL ciphers -v to see the cipher strings and their SSL
   * versions, key exchange, authentication, encryption, and message
   * digest algorithms, and key length restrictions.  See the OpenSSL
   * for the syntax to combine ciphers, e.g. !SSLv2:RC4-MD5:RC4-SHA.
   * If you don't specify anything, you get the same as "DEFAULT", which
   * means "ALL:!ADH:RC4+RSA:+HIGH:+MEDIUM:+LOW:+SSLv2:+EXP".
   */
  if(m->cipher) {
    if(!SSL_CTX_set_cipher_list(m->ctx, m->cipher)) {
      tb_warn("tb_inittSSL: Cannot use cipher list %s\n", m->cipher);
			goto err;
    }
		tb_info("tb_initSSL: cipher set to <%s>\n", m->cipher);
	}


	tb_info("tb_initSSL out\n");
	return TB_OK;

 err:
	// fixme: context not totally freed (CA_path, pwd ...)
	SSL_CTX_free(m->ctx);
	m->ctx = NULL;
	XSock(S)->status = TB_BROKEN;
	return TB_ERR;
}


int tb_connectSSL( Socket_t S ) {
  BIO* sbio=NULL,* dbio;
	int l;
	sock_ssl_t m = XSsl(S);

	tb_info("tb_connectSSL in\n");

  /* Ordinarily, just decorate the SSL connection with the socket file
   * descriptor.  But for super-cool shazaam debugging, build your
   * own socket BIO, decorate it with a BIO debugging callback, and
   * presto, see a dump of the bytes as they fly.  See ssl/ssl_lib.c
   * for details.  
   */
  if (tb_errorlevel <= TB_NOTICE) { 
    SSL_set_fd(m->cx, tb_getSockFD(S));
	} else {
    if (!(sbio=BIO_new_socket(tb_getSockFD(S),BIO_NOCLOSE))) {
      tb_warn("tb_connectSSL: Cannot create new socket BIO\n");
      ERR_print_errors_fp(stderr);
      goto err;
    }
    SSL_set_bio(m->cx,sbio,sbio);
    m->cx->debug=1;
    dbio=BIO_new_fp(stdout,BIO_NOCLOSE);
    BIO_set_callback(sbio,(void *)bio_dump_cb);
    BIO_set_callback_arg(sbio,dbio);
  }
	tb_info("ssl cx linked to socket\n");

  /* Initialize the state of the connection so the first i/o operation
   * knows to do the SSL connect.  Strictly speaking, this not necessary,
   * as this code goes ahead and calls SSL_connect() anyway (so I can 
   * put the connect tracing stuff in one handy spot).  But look closely
   * in ssl/s_client.c for a call to SSL_connect.  You won't find one.
   */
  SSL_set_connect_state(m->cx);
	//	SSL_CTX_set_session_cache_mode(XSSL(S)->ctx, SSL_SESS_CACHE_CLIENT);
	if(m->session != NULL ) {
		if(! SSL_set_session(m->cx, m->session)) {
			ERR_print_errors_fp(stderr);
		}
		tb_notice("tb_ConnectSSL: got a session to reuse ! (%X)\n", m->session);
	}


  /* Now that we've finally finished customizing the context and the
   * connection, go ahead and see if it works.  This function call 
   * invokes all the SSL connection handshake and key exchange logic,
   * (which is why there's so much to report on after it completes).
   */

 retry_connect:
  l = SSL_connect(m->cx);

	tb_info("ssl connect returns: %d\n", l);

  switch (SSL_get_error(m->cx,l)) { 
  case SSL_ERROR_NONE:
    break;
  case SSL_ERROR_SYSCALL:
    if ((l != 0) && errno)  tb_warn("tb_connectSSL: Write errno=%d\n", errno);
    goto err;
    break;
    /* fall through */
  case SSL_ERROR_WANT_WRITE:
		tb_info("SSL_ERROR_WANT_WRITE\n");
		goto retry_connect;
  case SSL_ERROR_WANT_READ:
		tb_info("SSL_ERROR_WANT_READ\n");
		goto retry_connect;
  case SSL_ERROR_WANT_X509_LOOKUP:
		tb_info("SSL_ERROR_WANT_X509_LOOKUP\n");
		goto retry_connect;
  case SSL_ERROR_ZERO_RETURN:
		tb_info("SSL_ERROR_ZERO_RETURN\n");
  case SSL_ERROR_SSL:
		tb_info("SSL_ERROR_SSL\n");
  default:
    ERR_print_errors_fp(stderr);
    goto err;
    break;
  }


	tb_info("connected\n");

	if(m->session == NULL ) {
		m->session = SSL_get_session(m->cx); //fixme: will leak !
		tb_notice("save session for later reuse (%X)\n", m->session);
		
	}

  /* Report on what happened now that we've successfully connected. */
  if (tb_errorlevel >= TB_NOTICE) ssl_barf_out(S);
  
	tb_info("tb_connectSSL out\n");

	return TB_OK;
 err:
	tb_info("tb_connectSSL err out\n");
	tb_Clear(S);
	return TB_ERR;
}








void ssl_barf_out(Socket_t S) {
	BIO *ebio;
	char buf[BUFSIZ], *p;
	sock_ssl_t m = XSsl(S);

  if (tb_errorlevel >= TB_NOTICE) {
		STACK      * sk;

    if ((ebio=BIO_new(BIO_s_file())) == NULL) {
      tb_warn("Cannot create new BIO\n");
      ERR_print_errors_fp(stderr);
      return;
    }
    BIO_set_fp(ebio,stderr,BIO_NOCLOSE);
    if ((sk=(STACK *)SSL_get_peer_cert_chain(m->cx)) != NULL) {
			int i;
      BIO_printf(ebio,"---\nCertificate chain\n");
      for (i=0; i<sk_num(sk); i++) {
        X509_NAME_oneline(X509_get_subject_name((X509*)sk_value(sk,i)),buf,BUFSIZ);
        BIO_printf(ebio,"%2d s:%s\n",i,buf);
        X509_NAME_oneline(X509_get_issuer_name((X509 *)sk_value(sk,i)),buf,BUFSIZ);
        BIO_printf(ebio,"   i:%s\n",buf);
      }
    }
    BIO_printf(ebio,"---\n");
    if ((m->peer=SSL_get_peer_certificate(m->cx)) != NULL) {
      BIO_printf(ebio,"Peer certificate\n");
      PEM_write_bio_X509(ebio,m->peer);
      X509_NAME_oneline(X509_get_subject_name(m->peer),buf,BUFSIZ);
      BIO_printf(ebio,"subject=%s\n",buf);
      X509_NAME_oneline(X509_get_issuer_name(m->peer),buf,BUFSIZ);
      BIO_printf(ebio,"issuer=%s\n",buf);
    }
    else
      BIO_printf(ebio,"no peer certificate available\n");
    if (((sk=SSL_get_client_CA_list(m->cx)) != NULL) && (sk_num(sk) > 0)) {
			int i;
      BIO_printf(ebio,"---\nAcceptable peer certificate CA names\n");
      for (i=0; i<sk_num(sk); i++) {
        m->xn=(X509_NAME *)sk_value(sk,i);
        X509_NAME_oneline(m->xn,buf,sizeof(buf));
        BIO_write(ebio,buf,strlen(buf));
        BIO_write(ebio,"\n",1);
      }
    }
    else {
      BIO_printf(ebio,"---\nNo peer certificate CA names sent\n");
    }
    if ((p=SSL_get_shared_ciphers(m->cx,buf,BUFSIZ)) != NULL) {
			int i, j;
      BIO_printf(ebio,"---\nCiphers common between both SSL endpoints:\n");
      j=i=0;
      while (*p) {
        if (*p != ':') {
          BIO_write(ebio,p,1);j++;
        } else {
          BIO_write(ebio,"                ",15-j%25);i++;j=0;
          BIO_write(ebio,((i%3)?" ":"\n"),1);
        }
        p++;
      }
      BIO_write(ebio,"\n",1);
    }
    BIO_printf(ebio,
               "---\nSSL handshake has read %ld bytes and written %ld bytes\n",
               BIO_number_read(SSL_get_rbio(m->cx)),
               BIO_number_written(SSL_get_wbio(m->cx)));
    BIO_printf(ebio,((m->cx->hit)?"---\nReused, ":"---\nNew, "));
    m->sc=SSL_get_current_cipher(m->cx);
    BIO_printf(ebio,"%s, Cipher is %s\n",
               SSL_CIPHER_get_version(m->sc),SSL_CIPHER_get_name(m->sc));
    if(m->peer != NULL) {
      EVP_PKEY *pktmp;
      pktmp = X509_get_pubkey(m->peer);
      BIO_printf(ebio,"Server public key is %d bit\n", EVP_PKEY_bits(pktmp));
      EVP_PKEY_free(pktmp);
    }
    SSL_SESSION_print(ebio,SSL_get_session(m->cx));
    BIO_printf(ebio,"---\n");
    if(m->peer != NULL)
      X509_free(m->peer);
    BIO_free(ebio);
  }
}

void tb_SSL_validate(Socket_t S, int bool, ssl_validate_cb_t validate) {
	ssl_validate_cb_t valid_fnc;
	if( validate != NULL) {
		valid_fnc = validate;
	} else {
		valid_fnc = (void *)verify_cb;
	}
	if( TB_VALID(S, TB_SOCKET) && XSsl(S) ) {
		if( bool ) {
			SSL_CTX_set_verify(XSsl(S)->ctx, SSL_VERIFY_PEER, (void *)valid_fnc);
		} else {
			SSL_CTX_set_verify(XSsl(S)->ctx, SSL_VERIFY_NONE, (void *)valid_fnc);
		}
	}
}


#endif
