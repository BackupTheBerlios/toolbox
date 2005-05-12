
#include "Toolbox.h"
#include "XmlRpc.h"
char remote1[] = "<Rpc methodName=\"func1\"><paramOut type=\"struct\"/><paramIn type=\"string\"/></Rpc>";
char remote2[] = "<Rpc methodName=\"func2\"><paramIn type=\"struct\"/><paramOut type=\"string\"/></Rpc>";

int main(int argc, char **argv) {
	XmlRpc_t Xr  = XmlRpc();
	Hash_t H     = tb_Hash();
	String_t S   = tb_String("totoche");
	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);
	int rc; 

	XRpc_registerRemoteMethod(Xr, remote1);
	XRpc_registerRemoteMethod(Xr, remote2);
	tb_Connect(So, 1, 1);
	
	tb_profile("calling...\n");
	if((rc = XRpc_sendCall(Xr, So, "func1", H, S)) == TB_OK) {
		tb_profile("Success: got reply [%s]\n", tb_toStr(tb_Stringify(H)));
	} else {
		tb_profile("Fault [%d]\n", rc);
	}

	if((rc = XRpc_sendCall(Xr, So, "func2", H, S)) == TB_OK) {
		tb_profile("Success: got reply [%s]\n", tb_toStr(tb_Stringify(H)));
	} else {
		tb_profile("Fault [%d]\n", rc);
	}


	tb_Free(So);
	tb_Free(S);
	tb_Free(Xr);

	return 0;
}
