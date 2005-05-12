
#define TB_MEM_DEBUG

#include <unistd.h>

#include "Toolbox.h"
#include "XmlRpc.h"

int stopFlag;

char remote1[] = "<Rpc methodName=\"func1\"><paramOut type=\"struct\"/><paramIn type=\"string\"/></Rpc>";

int func1(Hash_t H, String_t S) {
	//	tb_warn("called with N<%d> and S<%S>\n", tb_toInt(N), S);
	tb_Clear(H);
	tb_Clear(S);
	tb_StrAdd(S, 0, "ok");
	tb_Insert(H, tb_String("c'est un string"), "string_member");
	tb_Insert(H, tb_Num(42), "int_member");
	Hash_t H2 = tb_Hash();
	tb_Insert(H2, tb_String("c'est un autre string"), "one_more_string_member");
	tb_Insert(H2, tb_Num(442), "another_int_member");
	tb_Insert(H, H2, "table_de_hash");
	
	//	fm_Dump();

	stopFlag++;
	return TB_OK;
}

int main(int argc, char **argv) {
	XmlRpc_t Xr  = XmlRpc();
	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);
	stopFlag = 0;
	pthread_t pt;

	XRpc_registerLocalMethod(Xr, remote1, func1);

	tb_initServer(So, XRpc_receiveCall, Xr);

	pthread_create(&pt, NULL, tb_Accept, So); 
	pthread_detach(pt);

	while(stopFlag !=10) {
		sleep(1);
	}

	tb_stopServer(So);
	tb_Free(So);
	tb_Free(Xr);

	tb_profile("cleanup done");
	fm_Dump();
	fm_dumpChunks();
	return 0;
}
