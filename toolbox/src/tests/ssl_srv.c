// $Id: ssl_srv.c,v 1.1 2004/05/12 22:05:14 plg Exp $
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

#include "Toolbox.h"
#include <openssl/x509.h>


int callback(void *arg) {
	Socket_t S = (Socket_t)arg;
	String_t Str = tb_String(NULL);

	tb_trace(TB_WARN, "Server: New cx \n");
	tb_setSockTO(S, 5*60, 0);
	tb_setServMAXTHR(S, 5);
	// -------------
	tb_readSock(S, Str, MAX_BUFFER);
	tb_trace(TB_WARN, "Server: recv<%s>\n", tb_toStr(Str));
	tb_writeSock(S, "BYE");
	tb_trace(TB_WARN, "Server: exit callback\n");
	tb_Free(Str);

	return TB_OK;
}



int main(int argc, char **argv) {
	Socket_t sslServ;
	tb_profile("Version: %s", tb_getVersion()); 
	tb_profile("Build: %s", tb_getBuild()); 

	tb_errorlevel = TB_INFO;

	// make a new socket as usal
	sslServ = tb_Socket(TB_TCP_IP, "localhost", 44443);
	// init specific ssl stuff
	tb_initSSL(sslServ,    // Socket_t name
						 SSL_SERVER, // mode : SSL_SERVER | SSL_CLIENT
						 SSL3,       // ssl version (SSL2|SSL3|TLS1)
						 ".",               // rootdir or dir containing cert hashes (cf openssl) (optional)
						 "CA/ca.crt",       // fullpath to CA file  (optional)
						 "SRV/testserv.pem",// fullpath to server cert (w/ key) pem format (optional)
						 "testserv",        // private key passphrase (if any) 
						 NULL);             // cipher suite (optional)

																	 
	// setup validating policy (default is _validate_ allways)
	// to skip validation : uncomment next line
	tb_SSL_validate(sslServ, 0, NULL);

	// init server, hook callback, and prepare any ssl specific contexts
	tb_initServer(sslServ, callback, NULL);

	// start accepting incoming cx, and launch a callback thread for each one (blocking).
	tb_Accept(sslServ);
	
	return 0;
}


