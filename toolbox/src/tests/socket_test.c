// $Id: socket_test.c,v 1.3 2005/05/12 21:52:40 plg Exp $
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



void *callback(void *arg) {
	Socket_t S = (Socket_t)arg;
	String_t Str = tb_String(NULL);
	int rc;
	//char buff[100];

	tb_warn("Server: New cx \n");
	tb_Dump(S);

	// -- for educative purpose only. -optional- behaviour
	// setup timeout to 5mn 
	tb_setSockTO(S, 5*60, 0);
	// disable Naggle's algo (Don't mess with this if you don't known what's about)
	tb_setSockNoDelay(S);
	// -------------
	//tb_Dump(S);

	tb_readSock(S, Str, MAX_BUFFER);

	tb_warn("Server: recv<%s>\n", tb_toStr(Str));
	/*
	for(i=10;i>0; i--) {
		snprintf(buff, 100, "cb[%d] countdown: %d\n", pthread_self(), i);
		tb_writeSock(S, buff);
		sleep(1);
	}
	*/
	rc = tb_writeSock(S, "BYE");
	tb_warn("Server: wrote %d\n", rc);
	tb_Free(Str);

	return NULL;
}

void *tst_client(void *dummy) {
	Socket_t Client  = tb_Socket(TB_TCP_IP, "192.168.0.35", 55553);
	String_t S  = tb_String(NULL); 

	tb_Connect(Client, 1, 1);
	tb_setSockTO(Client, 20,0);
	if(tb_getSockStatus(Client) == TB_CONNECTED) {
		tb_writeSock(Client, "TEST TCP/IP");
		while( ! tb_matchRegex(S, "BYE", 0)) {
			if( tb_readSock(Client, S, 500) >0) {
				tb_warn("client: got <%s>\n", S2sz(S));
				tb_Clear(S);
			}
		}
	} else {
		tb_warn("Not connected !!\n");
	}
	
	tb_Free(S);

	return NULL;
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
	int test_tcp_unix = 1;

/* 	tb_profile("Version: %s", tb_getVersion());  */
/* 	tb_profile("Build: %s", tb_getBuild());  */

	if(test_udp_ip) {
		Socket_t Serv    = tb_Socket(TB_UDP_IP, "localhost", 55551);
		Socket_t Client  = tb_Socket(TB_UDP_IP, "localhost", 55552);

		String_t S  = tb_String(NULL); 
		pthread_t pt;

		tb_profile(" ------------------- test UDP/IP --------\n");
		tb_initServer(Serv, NULL, NULL);

		pthread_create(&pt, NULL, (void *)udp_ip, NULL); 
		pthread_detach(pt);

		sleep(1);

		tb_Connect(Client, 1, 1);
		
		if(tb_getSockStatus(Client) == TB_CONNECTED) {
			tb_writeSock(Client, "TEST UDP/IP");
			if( tb_readSock(Serv, S, 500) >0) {
				tb_warn("client: got <%S>\n", S);
			}
		} else {
			tb_warn("Not connected !!\n");
		}

		tb_Free(S);
		tb_Free(Serv);
		tb_Free(Client);
	}

	if(test_udp_unix) {
		Socket_t Serv    = tb_Socket(TB_UDP_UX, "/tmp/ux-sock.1", 0);
		Socket_t Client  = tb_Socket(TB_UDP_UX, "/tmp/ux-sock.2", 0);

		String_t S  = tb_String(NULL); 
		pthread_t pt;

		tb_profile(" ------------------- test UDP/UNIX --------\n");

		tb_Dump(Serv);
		tb_Dump(Client);
		tb_initServer(Serv, NULL, NULL);
		tb_Dump(Serv);

		pthread_create(&pt, NULL, (void *)udp_ux, NULL); 
		pthread_detach(pt);

		sleep(1);
		

		tb_Connect(Client, 1, 1);
		tb_Dump(Client);

		if(tb_getSockStatus(Client) == TB_CONNECTED) {
			tb_writeSock(Client, "TEST UDP/UNIX");
			if( tb_readSock(Serv, S, 500) >0) {
				tb_warn("client: got <%S>\n", S);
			}
		} else {
			tb_warn("Not connected !!\n");
		}

		tb_Free(S);
		tb_Free(Serv);
		tb_Free(Client);
	}

	if(test_tcp_ip) {
		//Socket_t Serv    = tb_Socket(TB_TCP_IP, "10.33.100.38", 55553);
		Socket_t Serv    = tb_Socket(TB_TCP_IP, "", 55553);
		//Socket_t Client  = tb_Socket(TB_TCP_IP, "10.33.100.38", 55553);
		//		Socket_t Client  = tb_Socket(TB_TCP_IP, "", 55553);
		//		String_t S  = tb_String(NULL); 
		pthread_t pt;
		int i;

		tb_errorlevel = TB_INFO;
		tb_profile(" ------------------- test TCP/IP --------\n");

		tb_initServer(Serv, callback, NULL);
		// setup max allowed threads to 5
		tb_setServMAXTHR(Serv, 5);


		pthread_create(&pt, NULL, tb_Accept, Serv); 
		pthread_detach(pt);

		for(i=0; i<1; i++) {
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

		sleep(12);
		tb_stopServer(Serv);
		tb_Free(Serv);
		//		tb_Free(Client);
	}

	if(test_tcp_unix) {
		Socket_t Serv    = tb_Socket(TB_TCP_UX, "/tmp/ux-sock.1", 0);
		Socket_t Client  = tb_Socket(TB_TCP_UX, "/tmp/ux-sock.1", 0);
		String_t S  = tb_String(NULL); 
		pthread_t pt;

		tb_profile(" ------------------- test TCP/UNIX --------\n");

		tb_initServer(Serv, callback, NULL);

		tb_Dump(Serv);
		tb_Dump(Client);


		pthread_create(&pt, NULL, tb_Accept, Serv); 
		pthread_detach(pt);

		sleep(2);
 
		tb_Connect(Client, 1, 1);
		
		if(tb_getSockStatus(Client) == TB_CONNECTED) {
			int rc;
			tb_warn("client: connected\n");
			rc = tb_writeSock(Client, "TEST TCP/UNIX");
			tb_warn("writesock : %d\n", rc);
			if((rc = tb_readSock(Client, S, 500)) >0) {
				tb_warn("client: got <%S>\n", S);
			} else {
				tb_warn("readsock : %d\n", rc);
			}
		} else {
			tb_warn("Not connected !!\n");
		}

		tb_stopServer( Serv );
		tb_Free( Serv );
		tb_Free( Client );
		tb_Free( S );
	}

	return 0;
}



