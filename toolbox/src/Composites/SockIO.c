//=======================================================
// $Id: SockIO.c,v 1.1 2004/05/12 22:04:49 plg Exp $
//=======================================================
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


#ifndef _REENTRANT
#  define _REENTRANT
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
#  define _POSIX_PTHREAD_SEMANTICS
#endif

#include "tb_global.h"

#ifdef AIX
#define _ALL_SOURCE
#endif

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

#include "Socket.h"
#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#ifdef AIX
#define PF_LOCAL AF_UNIX
#endif

#ifdef WITH_SSL
#include <ssl.h>
#endif
#include <arpa/inet.h>
#include "Socket.h"
#include "Toolbox.h"


/** read on a Socket_t using timeout and retries values as set in object.
 * \ingroup Socket
 * Target Socket_t must have been fully initialised by tb_Connect or tb_initServer. Read result is appended in 'msg' String_t.  
 *
 * @return number of read bytes, or -1 if error.
 * Example:
 * \code
 * ...
 * String_t S = tb_String(NULL);
 * Socket_t So = tb_Socket(TB_TCP_UX, "/tmp/my_unix_sock", 0);
 * tb_Connect(So, 1, 1);
 * int rc;
 *
 * while(( rc = tb_readSock(So, S, 1024)) > 0);
 * switch( rc ) {
 * case -1: // error occurs
 * case  0: // read reached eof (or timed out)
 * ...
 * \endcode
 * S will contains a concatened string of all 1024's buffers read 
 *
 * other Examples : 
 * see test/srv_test.c , test/socket_test.c in build tree
 *
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * - TB_ERR_BAD : msg is not a TB_STRING
 * 
 * @see tb_Socket, tb_writeSock, tb_writeSockBin, tb_readSockLine
*/
int tb_readSock(Socket_t S, tb_Object_t Msg, int maxlen) {
  fd_set set ;
  struct timeval tps ;
  int rc, retval = 0;
	sock_members_t So;
	char buff[maxlen+1];

	no_error;

	if(! TB_VALID(S,   TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}

	if(! TB_VALID(Msg, TB_STRING) && ! TB_VALID(Msg, TB_RAW)) {
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR;
	}

#ifdef WITH_XTI
	if( XSock(S)->addr_family   == TB_X25 ) {
		return tb_readSock_X25(S, Msg, maxlen);
	}
#endif


 
	So = XSock(S);


	if( So->buffer != NULL && tb_getSize(So->buffer) >0) {
		int n;
		n = TB_MIN( (maxlen), (tb_getSize(So->buffer)));
		tb_notice("readSock: already %d bytes in buffer\n", tb_getSize(So->buffer));
		if( n ) {
			if( tb_isA(Msg) == TB_STRING) {
				tb_StrnAdd(Msg, n, -1, "%S", So->buffer);
			} else {
			tb_RawAdd(Msg, n, -1, S2sz(So->buffer));
			}
			tb_StrDel(So->buffer, 0, n);
			if( n == maxlen) {
				return n;
			} else {
				maxlen -= n;
				retval = n;
			}
		}
	} 

	restart_r_select:

  FD_ZERO(&set);
  FD_SET(So->sock, &set);
  tb_getSockTO(S, &(tps.tv_sec), &(tps.tv_usec));

  rc = select(So->sock+1, &set, NULL, NULL, &tps);
  switch (rc)
    {
    case -1:
			if( errno == EINTR ) goto restart_r_select;
      tb_warn("tb_readSock[%d]: select failed (%s)\n", 
							 So->sock, strerror(errno)); 
      // invalid fd ==> we're disconnected
      if( errno == EBADF ) So->status = TB_DISCONNECTED;
			set_tb_errno(TB_ERR_DISCONNECTED);
      retval = TB_ERR;
      break; 
    case 0:
      /* Time out */
      tb_notice("tb_readSock[%d]: select timed out\n", So->sock); 
      So->status = TB_TIMEDOUT;
      retval = TB_KO;
			break ;
    default:
			if( ! FD_ISSET(So->sock, &set)) {
				tb_notice("tb_readSock[%d]: select rc=%d but fd is not ready\n", 
									So->sock, rc); 
				retval = TB_KO;
				break ;
			}
		restart_r_read:
#ifdef WITH_SSL
			if( So->ssl ) {
				tb_info("SSL_read: try to read %d bytes\n", maxlen);
				rc = SSL_read(XSsl(S)->cx, buff, maxlen);
				switch (SSL_get_error(XSsl(S)->cx,rc)) {
				case SSL_ERROR_NONE:
					buff[rc] = 0;
					tb_StrAdd(Msg, -1, "%s", buff);
					if( rc < maxlen ) {
						tb_info("SSL_read: got only %d/%d\n", rc, maxlen);
						maxlen -= rc;
						goto restart_r_select;
					}
					return rc;
				case SSL_ERROR_SYSCALL:
					if( errno ) {
						tb_warn("tb_readSock[%d(SSL)]: read error (%s)\n",
										 So->sock, strerror(errno));
					}
					/* fall through */
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_X509_LOOKUP:
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_SSL:
					ERR_print_errors_fp(stderr);
					return TB_ERR;
				}
			} else {
				set_nonblock_flag(So->sock, 1);
				rc = read(So->sock, buff, maxlen);
				set_nonblock_flag(So->sock, 0);
			}
#else
			set_nonblock_flag(So->sock, 1);
			rc = read(So->sock, buff, maxlen);
			set_nonblock_flag(So->sock, 0);
#endif			

      switch( rc ) {
      case -1:
				if( errno == EINTR ) goto restart_r_read;
				if( errno != EWOULDBLOCK ) {
					tb_error("tb_readSock[%d]: read error (%s)\n", So->sock, strerror(errno));
					retval = TB_ERR;
					So->status = TB_DISCONNECTED;
					break;
				}
      case 0:  // eof
				*buff = 0;
      default:
				buff[rc] = 0; 
				retval += rc;
				if(tb_isA(Msg) == TB_STRING) {
					tb_StrAdd(Msg, -1, "%s", buff);
				} else {
				tb_RawAdd(Msg, rc+1, -1, buff); 
				}
				break ;
      }
      break;
    }
  return retval;
}




/** write on a Socket_t using timeout and retries values as set in object.
 * \ingroup Socket
 * Target Socket_t must have been fully initialised by tb_Connect or tb_initServer. 
 *
 * @return number of written bytes, or -1 if error
 *
 * see test/srv_test.c , test/socket_test.c in build tree
 *
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * - TB_ERR_BAD : msg is not a TB_STRING
 * 
 * @see tb_Socket, tb_writeSock, , tb_writeSockBin, tb_readSockLine
*/
int tb_writeSock(Socket_t S, char *msg) {
  fd_set set ;
  struct timeval tps ;
  int len, rc, retval = 0;
	sock_members_t So;

	if(msg == NULL) {
		set_tb_errno(TB_ERR_BAD);
		return TB_KO;
	}
	
	if(!TB_VALID(S, TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}

#ifdef WITH_XTI
	if( XSock(S)->addr_family == TB_X25 ) {
		return tb_writeSock_X25(S, msg);
	}
#endif



	So = XSock(S);

	if(tb_getSockStatus(S) <= TB_DISCONNECTED) {
		return TB_ERR;
	}

 restart_w_select:
  FD_ZERO(&set);
  FD_SET(So->sock,&set);

  tb_getSockTO(S, &tps.tv_sec, &tps.tv_usec);

  rc = select(FD_SETSIZE, NULL, &set, NULL, &tps);
  switch(rc)
    {
    case -1: 
			if( errno == EINTR ) goto restart_w_select;
      tb_error("tb_writeSock[%d]: select failed (%s)\n", So->sock, strerror(errno)); 
      // invalid fd ==> we're disconnected
      if( errno == EBADF    ||
					errno == ENOTSOCK ||
					errno == ENOTCONN ||
					errno == EPIPE ) So->status = TB_DISCONNECTED;
      retval = TB_ERR;
      break ;
    case 0:
      /* Time out */
      tb_warn("tb_writeSock[%d]: select timed out\n", So->sock); 
      So->status = TB_TIMEDOUT;
      retval = TB_KO;
      break ;
    default:
			if( ! FD_ISSET(So->sock, &set)) {
				tb_error("tb_readSock[%d]: select rc=%d but fd is not ready\n", 
								 So->sock, rc); 
				retval = TB_KO;
				break ;
			}

		restart_w_write:
#ifdef WITH_SSL
			if( So->ssl ) {
				rc=SSL_write(XSsl(S)->cx,msg,strlen(msg));
				switch (SSL_get_error(XSsl(S)->cx,rc)) { 
				case SSL_ERROR_NONE:
					return rc;
				case SSL_ERROR_SYSCALL:
					if ((rc != 0) && errno ) {
						tb_error("tb_writeSock[%d(SSL)]: write error (%s)\n", 
										 So->sock, strerror(errno));
					}
					/* fall through */
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_X509_LOOKUP:
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_SSL:
				default:
					ERR_print_errors_fp(stderr);
					return TB_ERR;
				}
			} else {
				set_nonblock_flag(So->sock, 1);
				rc = write(So->sock, msg, len = strlen(msg));
				set_nonblock_flag(So->sock, 0);
			}
#else
			set_nonblock_flag(So->sock, 1);
			rc = write(So->sock, msg, len = strlen(msg));
			set_nonblock_flag(So->sock, 0);
#endif
      switch( rc ) {
      case -1:
				if( errno == EINTR ) goto restart_w_write;
				tb_error("tb_writeSock[%d]: write error (%s)\n", So->sock, strerror(errno));
				So->status = TB_DISCONNECTED;
				retval = TB_ERR;
				break;
      default:
				retval += rc;
				if( rc != strlen(msg) ) {
					tb_error("tb_writeSock[%d]: incomplet write (%d/%d)\n", So->sock, rc, len); 
					msg += rc;
					goto restart_w_select;
				}
				break ;
      }
      break ;
    }

  return retval;
}


/** write binary on a Socket_t using timeout and retries values as set in object.
 * \ingroup Socket
 * Target Socket_t must have been fully initialised by tb_Connect or tb_initServer. 
 *
 * @return number of written bytes, or -1 if error
 *
 * see test/srv_test.c , test/socket_test.c in build tree
 *
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * - TB_ERR_BAD : msg is not a TB_STRING
 * 
 * @see tb_Socket, tb_writeSock, , tb_writeSockBin, tb_readSockLine
 * \todo fixme: use a pointer and a counter to remaining bytes to write for icomplete writes ...
 */
int tb_writeSockBin(Socket_t S, Raw_t raw) {
  fd_set set ;
  struct timeval tps ;
  int len, rc, remaining, retval = 0;
	char *msg;
	sock_members_t So;

	if(!TB_VALID(raw, TB_RAW)) {
		set_tb_errno(TB_ERR_BAD);
		return TB_KO;
	}
	
	if(!TB_VALID(S, TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}

	So = XSock(S);

	if(tb_getSockStatus(S) <= TB_DISCONNECTED) {
		return TB_ERR;
	}

  FD_ZERO(&set);
  FD_SET(So->sock,&set);

	msg = S2sz(raw);
	remaining = tb_getSize(raw);

 restart_w_select:
  tb_getSockTO(S, &tps.tv_sec, &tps.tv_usec);

  rc = select(FD_SETSIZE, NULL, &set, NULL, &tps);
  switch(rc)
    {
    case -1: 
			if( errno == EINTR ) goto restart_w_select;
      tb_error("tb_writeSock[%d]: select failed (%s)\n", So->sock, strerror(errno)); 
      // invalid fd ==> we're disconnected
      if( errno == EBADF    ||
					errno == ENOTSOCK ||
					errno == ENOTCONN ||
					errno == EPIPE ) So->status = TB_DISCONNECTED;
      retval = TB_ERR;
      break ;
    case 0:
      /* Time out */
      tb_warn("tb_writeSock[%d]: select timed out\n", So->sock); 
      So->status = TB_TIMEDOUT;
      retval = TB_KO;
      break ;
    default:
			if( ! FD_ISSET(So->sock, &set)) {
				tb_error("tb_readSock[%d]: select rc=%d but fd is not ready\n", 
								 So->sock, rc); 
				retval = TB_KO;
				break ;
			}

		restart_w_write:
#ifdef WITH_SSL
			if( So->ssl ) {
				rc=SSL_write(XSsl(S)->cx,msg , remaining);
				switch (SSL_get_error(XSsl(S)->cx,rc)) { 
				case SSL_ERROR_NONE:
					return rc;
				case SSL_ERROR_SYSCALL:
					if ((rc != 0) && errno ) {
						tb_error("tb_writeSock[%d(SSL)]: write error (%s)\n", 
										 So->sock, strerror(errno));
					}
					/* fall through */
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_X509_LOOKUP:
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_SSL:
				default:
					ERR_print_errors_fp(stderr);
					return TB_ERR;
				}
			} else {
				set_nonblock_flag(So->sock, 1);
				rc = write(So->sock, msg, len = remaining);
				set_nonblock_flag(So->sock, 0);
			}
#else
			set_nonblock_flag(So->sock, 1);
			rc = write(So->sock, msg, len = remaining);
			set_nonblock_flag(So->sock, 0);
#endif
      switch( rc ) {
      case -1:
				if( errno == EINTR ) goto restart_w_write;
				tb_error("tb_writeSock[%d]: write error (%s)\n", So->sock, strerror(errno));
				So->status = TB_DISCONNECTED;
				retval = TB_ERR;
				break;
      default:
				retval += rc;
				if( rc != remaining ) {
					tb_error("tb_writeSock[%d]: incomplet write (%d/%d)\n", So->sock, rc, len); 
					msg       += rc;
					remaining -= rc;
					goto restart_w_select;
				}
				break ;
      }
      break ;
    }

  return retval;
}




/** read a line on a Socket_t using timeout and retries values as set in object.
 * \ingroup Socket
 * Target Socket_t must have been fully initialised by tb_Connect or tb_initServer. Read result is appended in 'msg' String_t.  
 *
 * @return number of read bytes, or -1 if error
 * Internally read is buffered for best performances. The first line found in the buffer is appended to Msg String_t. The remaining buffered data (if any) will be used on next tb_readSockLine or tb_readSock.
 *
 * \remarks ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT : So not a TB_SOCKET
 * - TB_ERR_BAD : msg is not a TB_STRING
 * 
 * @see tb_Socket, tb_writeSock, tb_writeSockBin, tb_readSockLine
*/
int tb_readSockLine(Socket_t S, String_t Msg) {
  fd_set set ;
  struct timeval tps ;
  int rc, retval;
	sock_members_t So;
	char buff[MAX_BUFFER];
	no_error;

	if(! TB_VALID(S,   TB_SOCKET)) {
		set_tb_errno(TB_ERR_INVALID_TB_OBJECT);
		return TB_ERR;
	}
	if(! TB_VALID(Msg, TB_STRING)) {
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR;
	}

	So = XSock(S);

	if( So->buffer == NULL ) {
		So->buffer = tb_String(NULL);
	} else {
		if( tb_getSize(So->buffer) > 0 ) {
			char *s;
			if(( s = strchr(S2sz(So->buffer), '\n')) != NULL) {
				int len;

				len = 1+ (s - S2sz(So->buffer));
				tb_StrnAdd(Msg, len, -1, "%s", S2sz(So->buffer));
				tb_StrDel(So->buffer, 0, len);
				return len;
			}
		}
	}

  FD_ZERO(&set);
  FD_SET(So->sock, &set);
  tb_getSockTO(S, &(tps.tv_sec), &(tps.tv_usec));
 
	restart_r_select:
  rc = select(So->sock+1, &set, NULL, NULL, &tps);
  switch (rc)
    {
    case -1:
			if( errno == EINTR ) goto restart_r_select;
      tb_error("tb_readSockLine[%d]: select failed (%s)\n", 
							 So->sock, strerror(errno)); 
      // invalid fd ==> we're disconnected
      if( errno == EBADF ) So->status = TB_DISCONNECTED;
			set_tb_errno(TB_ERR_DISCONNECTED);
      retval = TB_ERR;
      break; 
    case 0:
      /* Time out */
      tb_warn("tb_readSockLine[%d]: select timed out\n", So->sock); 
      So->status = TB_TIMEDOUT;
      retval = TB_ERR;
      break ;
    default:
		restart_r_read:
#ifdef WITH_SSL
			if( So->ssl ) {
				rc = SSL_read(XSsl(S)->cx, buff, MAX_BUFFER-1);
				switch (SSL_get_error(XSsl(S)->cx,rc)) {
				case SSL_ERROR_NONE:
					buff[rc] = 0;
					tb_StrAdd(Msg, -1, "%s", buff);
					goto cutline;
				case SSL_ERROR_SYSCALL:
					if( errno ) {
						tb_error("tb_readSock[%d(SSL)]: read error (%s)\n",
										 So->sock, strerror(errno));
					}
					/* fall through */
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_READ:
				case SSL_ERROR_WANT_X509_LOOKUP:
				case SSL_ERROR_ZERO_RETURN:
				case SSL_ERROR_SSL:
					ERR_print_errors_fp(stderr);
					return TB_ERR;
				}
			} else {
				set_nonblock_flag(So->sock, 1);
				rc = read(So->sock, buff, MAX_BUFFER-1);
				set_nonblock_flag(So->sock, 0);
			}
#else
			set_nonblock_flag(So->sock, 1);
			rc = read(So->sock, buff, MAX_BUFFER-1);
			set_nonblock_flag(So->sock, 0);
#endif
#ifdef WITH_SSL
    cutline:
#endif // WITH_SSL
      switch( rc ) {
      case -1:
				if( errno == EINTR ) goto restart_r_read;
				if( errno != EWOULDBLOCK ) {
					tb_error("tb_readSockLine[%d]: read error (%s)\n", So->sock, strerror(errno));
					retval = TB_ERR;
					So->status = TB_DISCONNECTED;
					break;
				}
      case 0:  // eof
				tb_notice("tb_readSockLine[%d]: eof reached\n", So->sock);
				*buff = 0;
				tb_StrAdd(Msg, -1, "%S", So->buffer);
				retval = TB_ERR;
				tb_Clear(So->buffer);
				break;
      default:
				buff[rc] = 0; 
				tb_StrAdd(So->buffer, -1, "%s", buff);
				{
					char *s = strchr(S2sz(So->buffer), '\n');
					if( s ) {
						int len;
						len = 1+(s - S2sz(So->buffer)); 
						tb_StrnAdd(Msg, len, -1, "%s", S2sz(So->buffer));
						tb_StrDel(So->buffer, 0, len);
						retval = len;
					} else {
						tb_warn("tb_readSockLine[%d]: no full line found\n", So->sock);
						// added twice ???
						// tb_StrAdd(So->buffer, -1, "%s", buff);
						retval = strlen(buff);
					}
				}
				break;
      }
      break;
    }
  return retval;
}






