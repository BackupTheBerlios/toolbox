// $Id: socket_acl_test.c,v 1.1 2004/05/12 22:05:14 plg Exp $
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

extern void tb_dumpACL(Socket_t S);

int callback(void *arg) {
	Socket_t S = (Socket_t)arg;
	String_t Str = tb_String(NULL);
	//int i;
	//char buff[100];

	tb_trace(TB_WARN, "Server: New cx \n");
	tb_Dump(S);

	// -- for educative purpose only. -optional- behaviour
	// setup timeout to 5mn 
	tb_setSockTO(S, 5*60, 0);
	// disable Naggle's algo (Don't mess with this if you don't known what's about)
	tb_setSockNoDelay(S);
	// -------------
	//tb_Dump(S);

	tb_readSock(S, Str, MAX_BUFFER);

	tb_trace(TB_WARN, "Server: recv<%s>\n", tb_toStr(Str));
	/*
	for(i=10;i>0; i--) {
		snprintf(buff, 100, "cb[%d] countdown: %d\n", pthread_self(), i);
		tb_writeSock(S, buff);
		sleep(1);
	}
	*/
	sleep(1);
	tb_writeSock(S, "BYE");
	//tb_trace(TB_WARN, "Server: exit callback\n");
	tb_Free(Str);
	return TB_OK;
}

int tst_client(void) {
	Socket_t Client  = tb_Socket(TB_TCP_IP, "xanadu.dev33.cvf", 55553);
	String_t S  = tb_String(NULL); 

	tb_warn("in tst_client");

	tb_Connect(Client, 1, 1);
	tb_setSockTO(Client, 0,50000);
	if(tb_getSockStatus(Client) == TB_CONNECTED) {
		tb_warn("in tst_client: connected");
		if( tb_writeSock(Client, "TEST TCP/IP") <0) {
			tb_warn("in tst_client: cx broken");
			goto bad;
		}
		while( ! tb_matchRegex(S, "BYE", NULL, 0)) {
			if( tb_readSock(Client, S, 500) >0) {
				tb_warn("client: got <%s>\n", S2sz(S));
				tb_Clear(S);
			} else {
				tb_warn("in tst_client: cx broken");
				break;
			}
		}
	} else {
		tb_trace(TB_WARN, "Not connected !!\n");
	}
 bad:
	tb_Free(S);

	return 1;
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
	tb_Free(Str);
	tb_Free(Serv);
	tb_Free(Client);
		
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
	tb_Free(Str);
	tb_Free(Serv);
	tb_Free(Client);

	return 0;
}





int main(int argc, char **argv) {

	int test_udp_ip   = 0;
	int test_udp_unix = 0;
	int test_tcp_ip   = 1;
	int test_tcp_unix = 0;

	tb_profile("Version: %s", tb_getVersion()); 
	tb_profile("Build: %s", tb_getBuild()); 


	if(test_tcp_ip) {
		//Socket_t Serv    = tb_Socket(TB_TCP_IP, "10.33.100.38", 55553);
		Socket_t Serv    = tb_Socket(TB_TCP_IP, "xanadu.dev33.cvf", 55553);
		//Socket_t Client  = tb_Socket(TB_TCP_IP, "10.33.100.38", 55553);
		//		Socket_t Client  = tb_Socket(TB_TCP_IP, "", 55553);
		//		String_t S  = tb_String(NULL); 
		pthread_t pt;
		int i;

		tb_errorlevel = TB_INFO;
		tb_profile(" ------------------- test TCP/IP --------\n");

		//		tb_initServer(Serv, callback, NULL);

		tb_initServer(Serv, callback, NULL);

		tb_sockACL_ADD(Serv, ACL_DENY, "ALL");
		tb_sockACL_ADD(Serv, ACL_ALLOW, ".dev33.cvf");
		tb_sockACL_ADD(Serv, ACL_ALLOW, "127.0.0.1");
		tb_sockACL_set_global_max_hps(Serv, 8);
		tb_sockACL_set_host_max_simult(Serv, 2);

		tb_dumpACL(Serv);		

		tb_sockACL(Serv, 1);

		// setup max allowed threads to 5
		tb_setServMAXTHR(Serv, 5);


		pthread_create(&pt, NULL, tb_Accept, Serv); 
		pthread_detach(pt);

		sleep(1);

		for(i=0; i<10; i++) {
			usleep(50000);
			pthread_create(&pt, NULL, tst_client, NULL); 
			pthread_detach(pt);
		}

		sleep(2);
		tb_sockACL(Serv, 0);

		for(i=0; i<10; i++) {
			sleep(1);
			pthread_create(&pt, NULL, tst_client, NULL); 
			pthread_detach(pt);
		}


		/*
		tb_Connect(Client, 1, 1);
		
		if(tb_getSockStatus(Client) == TB_CONNECTED) {
			tb_writeSock(Client, "TEST TCP/IP");
			if( tb_readSock(Client, S, 500) >0) {
				tb_warn("client: got <%s>\n", S2sz(S));
			}
		} else {
			tb_trace(TB_WARN, "Not connected !!\n");
		}

		tb_Free(S);
		*/

		sleep(2);
		tb_stopServer(Serv);
		tb_Free(Serv);
		//		tb_Free(Client);
	}


	return 0;
}



