
#include "Toolbox.h"
#include "../XmlLite/XmlRpc.h"

char remote1[] = "<Rpc methodName=\"func1\"><paramIn type=\"int\"/><paramOut type=\"string\"/></Rpc>";

int main(int argc, char **argv) {
	XmlRpc_t Xr  = XmlRpc();
	Num_t N      = tb_Num(999);
	String_t S   = tb_String("totoche");
	int rc; 
	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);

	XRpc_registerRemoteMethod(Xr, remote1);
	tb_Connect(So, 1, 1);
	
	tb_profile("calling...\n");
	if((rc = XRpc_sendCall(Xr, So, "func1", N, S)) == TB_OK) {
		tb_profile("Success: got reply [%d] [%S]\n", tb_toInt(N), S);
	} else {
		tb_profile("Fault [%d]\n", rc);
	}

	tb_Free(So);
	tb_Free(S);
	tb_Free(N);
	tb_Free(Xr);

	return 0;
}
