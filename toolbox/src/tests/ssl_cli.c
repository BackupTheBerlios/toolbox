// $Id: ssl_cli.c,v 1.1 2004/05/12 22:05:14 plg Exp $
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
#include <sys/socket.h>
#include "Toolbox.h"

#include <openssl/x509.h>


/*
typedef void (*ssl_validate_cb_t)(int, X509_STORE_CTX *);
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
extern void tb_SSL_validate(Socket_t S, int bool, ssl_validate_cb_t validate);
*/

int main(int argc, char **argv) {
	int i;
	tb_profile("Version: %s", tb_getVersion()); 
	tb_profile("Build: %s", tb_getBuild()); 

	tb_errorlevel = TB_INFO;

	if(1) {
		Socket_t ssl  = tb_Socket(TB_TCP_IP, "localhost", 44443);
		String_t S  = tb_String(NULL); 
		tb_profile(" ------------------- test SSL --------\n");

		tb_initSSL( ssl, 
								SSL_CLIENT,
								SSL3, 
								".", 
								"CA/ca.crt", 
								"CLI/testcli.pem", // full path to pem file
								"testcli", 
								NULL);// cipher

		tb_SSL_validate(ssl, 0, NULL);		

		for(i=0;i<2;i++) {

			tb_Connect(ssl, 1, 1);
			tb_setSockTO(ssl, 10,10);

			if(tb_getSockStatus(ssl) == TB_CONNECTED) {
				char *s;
				tb_asprintf(&s, "SSL_TRY #%d\n", i);
				tb_warn("writesock rc: %d\n", tb_writeSock(ssl, s));
				shutdown(tb_getSockFD(ssl), 1);
				while( tb_readSock(ssl, S, 1024) >0 );
				tb_warn("readsock read %d bytes\n", tb_getSize(S));
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


