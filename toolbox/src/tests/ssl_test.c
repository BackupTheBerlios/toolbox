// $Id: ssl_test.c,v 1.1 2004/05/12 22:05:14 plg Exp $
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <syslog.h>

#include "Toolbox4.h"


extern RETCODE tb_SSL_initSockServer(Socket_t S, 
															void *callback, // int(*)(Socket_t)
															void *cb_args,
															ssl_meth_t method, 
															char *CA_path,
															char *CA_file,
															char *cert,
															char *pwd,
															char *cipher);
extern void *tb_SSL_Accept( void *Arg );

int callback(void *arg) {
	Socket_t S = (Socket_t)arg;
	String_t Str = tb_String(NULL);

	tb_trace(TB_WARN, "Server: New cx \n");
	tb_Dump(S);

	// -- for educative purpose only. -optional- behaviour
	// setup timeout to 5mn 
	tb_setSockTO(S, 5*60, 0);
	// setup max allowed threads to 5
	tb_setServMAXTHR(S, 5);
	// disable Nagle's algo
	tb_setSockNoDelay(S);
	// -------------
	tb_Dump(S);

	tb_readSock(S, Str, MAX_BUFFER);

	tb_trace(TB_WARN, "Server: recv<%s>\n", tb_toStr(Str));
	tb_writeSock(S, "BYE");
	tb_trace(TB_WARN, "Server: exit callback\n");
	tb_Free(Str);

	return TB_OK;
}

int udp_ip(void *arg) {
	Socket_t Serv    = tb_Socket(TB_UDP_IP, "localhost", 55552);
	Socket_t Client  = tb_Socket(TB_UDP_IP, "localhost", 55551);
	String_t Str = tb_String(NULL);
	int rc;
	tb_initServer(Serv, NULL, NULL);
	tb_Connect(Client, 1, 1);
						 
	tb_Dump(Serv);
  tb_setSockTO(Serv, 15, 0);
	rc = tb_readSock(Serv, Str, MAX_BUFFER);
	if( rc != 0 ) {
		tb_warn("UDP/IP Server: got <%S>\n", Str);
		tb_writeSock(Client, "UDP/IP:BYE"); 
		tb_Clear(Str);
	}
	tb_warn("UDP/IP Server exiting\n");
		
	return 0;
}

int udp_ux(void *arg) {
	Socket_t Serv    = tb_Socket(TB_UDP_UX, "/tmp/ux-sock.2", 0);
	Socket_t Client  = tb_Socket(TB_UDP_UX, "/tmp/ux-sock.1", 0);
	String_t Str = tb_String(NULL);
	int rc;
	tb_initServer(Serv, NULL, NULL);
	tb_Connect(Client, 1, 1);
						 
	tb_Dump(Serv);
	tb_Dump(Client);
  tb_setSockTO(Serv, 15, 0);
	rc = tb_readSock(Serv, Str, MAX_BUFFER);
	if( rc != 0 ) {
		tb_warn("UDP/UX Server: got <%S>\n", Str);
		tb_writeSock(Client, "UDP/UNIX:BYE");
		tb_Clear(Str);
	}
	tb_warn("UDP/UX Server exiting\n");
		
	return 0;
}



int main(int argc, char **argv) {

	int test_ssl   = 1;

	int i;
	tb_profile("Version: %s", tb_getVersion()); 
	tb_profile("Build: %s", tb_getBuild()); 

	tb_errorlevel = TB_DEBUG;

	if(test_ssl) {
		//		Socket_t ssl  = tb_Socket(TB_TCP_IP, "spg-test.aix75.cvf", 443);
		Socket_t sslServ  = tb_Socket(TB_TCP_IP, "", 44443);
		Socket_t ssl  = tb_Socket(TB_TCP_IP, "mail.orange.fr", 443);
		String_t S  = tb_String(NULL); 
		tb_profile(" ------------------- test SSL --------\n");

		//		tb_initSSL( ssl, SSL3, ".", "ca.crt", "client1.crt", "client1.key", NULL);
		//		tb_initSSL( ssl, SSL3, ".", "ca.crt", "wrong.crt", "wrong.key", NULL);
		/*
		tb_initSSL( ssl, 
								SSL3, 
								"/tmp", 
								//"wrong.crt", 
								"tutu/ca.crt", 
								"/tmp/client1.pem", // full path to pem file
								"webhelp", 
								NULL);// cipher
		*/

		tb_SSL_initSockServer(sslServ, callback, NULL, 
											SSL3, ".", "ca.crt", "wrong.pem", "ppasswd", NULL);
		tb_SSL_Accept(sslServ);


		tb_initSSL( ssl, 
								SSL3, 
								"/", 
								//"wrong.crt", 
								"verisign.pem", 
								NULL, // "/tmp/client1.pem", // full path to pem file
								NULL, //"webhelp", 
								NULL);// cipher



		for(i=1;i<2;i++) {

			tb_Connect(ssl, 1, 1);
			tb_setSockTO(ssl, 10,10);
		
			if(tb_getSockStatus(ssl) == TB_CONNECTED) {
				tb_profile("writesock rc: %d\n", tb_writeSock(ssl, "GET /iwsroot/main_logoff_admlogin.asp"));
				shutdown(tb_getSockFD(ssl), 1);
				while( tb_readSock(ssl, S, 1024) >0 );
				tb_profile("readsock read %d bytes\n", tb_getSize(S));
				fprintf(stderr, "client: got <%s>\n", S2sz(S));
				tb_profile(" ------------------- test SSL done --------\n");
			} else {
				tb_trace(TB_WARN, "Not connected !!\n");
			}
			tb_Clear(ssl);
			tb_Clear(S);
		}
	}


	return 0;
}


