// $Id: srv_test.c,v 1.1 2004/05/12 22:05:14 plg Exp $
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

int callback(void *arg) {
	Socket_t S = (Socket_t)arg;
	String_t Str = tb_String(NULL);
	int i;

	tb_trace(TB_WARN, "Server: New cx (%p)\n", arg);
	tb_Dump(S);
	fflush(stderr);
	tb_readSock(S, Str, MAX_BUFFER);

	tb_trace(TB_WARN, "Server: recv<%S>\n", Str);

	for(i=10; i>0; i--) {
		char buf[256];
		snprintf(buf, 255, "cb[%d] countdown: %d ...\n", pthread_self(), i);
		sleep(1);
		tb_writeSock(S, buf);
	}
	tb_writeSock(S, "BYE");
	
	tb_trace(TB_WARN, "Server: exit callback\n");
	tb_Free(Str);

	return TB_OK;
}



int main(int argc, char **argv) {
	Hash_t H;
	Socket_t Server = tb_Socket(TB_TCP_IP, "localhost", 5555);

	pthread_t pt;
	pthread_attr_t attr;
	char buff[MAX_BUFFER+1];
	int i, rc;
	char datum[] = "hello shiny world !";
	int acks = 0;
	int max = 5000;

	tb_errorlevel = TB_INFO;

	tb_setServMAXTHR(Server, 5);

	if((rc = tb_initServer(Server, callback, NULL)) != TB_OK) {
		tb_trace(TB_WARN, "init server failed\n");
		return 1;
	}

	tb_Dump(Server);

	tb_Accept( Server );

	return 0;
}




