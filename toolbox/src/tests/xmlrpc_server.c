

#include "Toolbox.h"
#include "../XmlLite/XmlRpc.h"

char remote1[] = "<Rpc methodName=\"func1\"><paramIn type=\"int\"/><paramOut type=\"string\"/></Rpc>";

int func1(Num_t N, String_t S) {
	tb_warn("server: func1 called with N<%d> and S<%S>\n", tb_toInt(N), S);
	tb_StrAdd(S, 0, "Hello ");
	tb_StrAdd(S, -1, " :-)");
	
	return TB_OK;
}

int main(int argc, char **argv) {
	XmlRpc_t Xr  = XmlRpc();
	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);


	XRpc_registerLocalMethod(Xr, remote1, func1);

	tb_initServer(So, XRpc_receiveCall, Xr);

	tb_Accept(So);


	return 0;
}
