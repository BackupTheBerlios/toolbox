//=======================================================
// $Id: XmlRpc_util.c,v 1.3 2004/05/27 15:54:07 plg Exp $
//=======================================================
/* Copyright (c) 1999-2004, Paul L. Gatille <paul.gatille@free.fr>
 *
 * This file is part of Toolbox, an object-oriented utility library
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the "Artistic License" which comes with this Kit.
 *
 * This software is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the Artistic License for more
 * details.
 */


/*
<XmlRpc name="myFunc">
  <paramIn name="size" type="int"/>
	<paramInOut name="state" type="string"/>
</XmlRpc>




<?xml version="1.0"?>
<methodCall>
   <methodName>myFunc</methodName>
   <params>
      <param>
         <value><int>41</int></value>
      </param>
      <param>
         <value><string>voila</string></value>
      </param>
   </params>
</methodCall>


--->


<?xml version="1.0"?>
<methodResponse>
   <params>
      <param>
         <value><string>South Dakota</string></value>
      </param>
   </params>
</methodResponse>
*/

#include <sys/socket.h> // for shutdown
#include <stdio.h>

#include "Toolbox.h"
#include "XmlRpc.h"

static int  XRpc_callNative          (XmlRpc_t Xr, char *methodName, Vector_t argVector);
static void XRpc_makeFaultResponse   (String_t Response, int fault);


typedef int (*_p1__t)(tb_Object_t);
typedef int (*_p2__t)(tb_Object_t,tb_Object_t);
typedef int (*_p3__t)(tb_Object_t,tb_Object_t,tb_Object_t);
typedef int (*_p4__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t);
typedef int (*_p5__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t,tb_Object_t);
typedef int (*_p6__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t,tb_Object_t,tb_Object_t);
typedef int (*_p7__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t);
typedef int (*_p8__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t);
typedef int (*_p9__t)(tb_Object_t,tb_Object_t,tb_Object_t,
								tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t,tb_Object_t);


/**
 * Invoke remote function, using XmlRpc specs for serialising/deserializing
 *
 * Called function must have been registered from both sides (client & server).
 *
 * Using is straightforward : 
 *
 * 1/ register Rpc client and server side using XRpc_registerRemoteMethod and XRpc_registerLocalMethod
 *
 * 2/ Open Socket_t to remote server
 *
 * 3/ call RPC using XRpc_sendCall, with the XmlRpc object, the open Socket_t, the remote function name (as registered), and all requiered parameters.
 *
 * Internally, all parameters are checked against registered signature, and serialized. They are then sent to remote part.
 *
 * Remote server accepts the serialized call, checks it also with it's own signature, deserialize the arguments (in other words : create new objects from their textual representation) and call the rela function. According to the return code, the remote part will send back a Fault structure, or (in case of success) will re-serialise all args marked as return values ('Out' and 'InOut' in signature) and send back to caller.
 *
 * Caller will then update calling parameters with the textual representation of the return values.
 * All those steps are done transparently.
 * 
 * Example : 
 *\code
 * Client side :
 * char remote1[] = "<Rpc methodName=\"func1\"><paramIn type=\"int\"/><paramOut type=\"string\"/></Rpc>";
 *
 * int main(int argc, char **argv) {
 *	XmlRpc_t Xr  = XmlRpc();                          // new XmlRpc object 
 *	Num_t N      = tb_Num(999);                       // define some objects matching signature
 *	String_t S   = tb_String("totoche"); 
 *	int rc; 
 *	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);   // create a socket to remote part
 *
 *	XRpc_registerRemoteMethod(Xr, remote1);           // register signature
 *	tb_Connect(So, 1, 1);                             // open connection
 *
 *  rc = XRpc_sendCall(Xr, So, "func1", N, S)         // execute remote call
 *
 *	if(rc == TB_OK) {
 *		fprintf(stderr, "Success: %s\n", tb_toStr(S));  // print return value
 *	} else {
 *		fprintf(stderr, "Fault [%d]\n", rc);            // something gone wrong
 *	}
 *  ...
 * }
 * \endcode
 *
 *\code
 * Server side :
 * char remote1[] = "<Rpc methodName=\"func1\"><paramIn type=\"int\"/><paramOut type=\"string\"/></Rpc>";
 *
 * int func1(Num_t N, String_t S) {
 * 	tb_warn("server: func1 called with N<%d> and S<%S>\n", tb_toInt(N), S);
 * 	tb_StrAdd(S, 0, "Hello ");
 * 	tb_StrAdd(S, -1, " :-)");
 *	
 *	return TB_OK;
 * }
 *
 * int main(int argc, char **argv) {
 *	XmlRpc_t Xr  = XmlRpc();                         // create XmlRpc object
 *	Socket_t  So = tb_Socket(TB_TCP_IP, "", 55553);  // create socket endpoint
 *
 *	XRpc_registerLocalMethod(Xr, remote1, func1); // register signature and local function pointer
 *
 *	tb_initServer(So, XRpc_receiveCall, Xr);         // prepare server to accept XmlRpc Calls
 *
 *	tb_Accept(So);                                   // start accpepting
 *
 *	return 0;                                        // that's all folks
 * }
 * \endcode

 * @ingroup Xmlrpc
 */
int XRpc_sendCall(XmlRpc_t Xr, /*Uri_t Dest*/ Socket_t So, char *func, ...) {
	int rc;
	XmlDoc_t Sig         = XRpc_getSignature(Xr, func);
	if(Sig) {
		Iterator_t It      = tb_Iterator(XELT_getChildren(XDOC_getRoot(Sig)));
		if(It) {
			va_list parms;
			XmlDoc_t ResponseDoc;
			XmlElt_t Response;

			Vector_t outVector = tb_Vector();
			String_t Request   = tb_String("%s", "<?xml version=\"1.0\"?>\n<methodCall>\n");
			va_start(parms, func);

			tb_StrAdd(Request, -1, "  <methodName>%S</methodName>\n", XRpc_getMethodName(XDOC_getRoot(Sig)));
			tb_StrAdd(Request, -1, "  <params>\n");
		
			do {
				String_t Tmp;
				tb_Object_t O = (tb_Object_t)va_arg(parms, tb_Object_t);
				if(tb_valid(O, XRpc_getParamType(tb_Value(It)), __FUNCTION__)) {
					int style = XRpc_getParamStyle(tb_Value(It));

					// serialise all parameters
					tb_chomp(Tmp = tb_Marshall(O));
					tb_StrAdd(Request, -1, "    <param><value>%S</value></param>\n", Tmp);
					tb_Free(Tmp);

					// save Out parameters for storing return values
					if(style == XMLRPC_PARAM_INOUT || style == XMLRPC_PARAM_OUT) {
						tb_Push(outVector, tb_Alias(O));
					}

				} else {
					tb_Free(outVector);
					tb_Free(It);
					tb_Free(Request);
					return XMLRPC_FAULT_TYPE_MISMATCH;
				}
			} while(tb_Next(It));
			tb_Free(It);
		
			tb_StrAdd(Request, -1, "  </params>\n");
			tb_StrAdd(Request, -1, "</methodCall>\n");

			fprintf(stderr, "%s", tb_toStr(Request));

			//		So = Uri_openIOChannel(Dest);
			rc = tb_writeSock(So, tb_toStr(Request));
			tb_Free(Request);

			if(rc != TB_ERR && rc != TB_KO) {
				String_t Reply = tb_String(NULL);
				
				shutdown(tb_getSockFD(So), SHUT_WR); // cut write uplink
				while(tb_readSock(So, Reply, MAX_BUFFER) >0);

				ResponseDoc = tb_XmlDoc(tb_toStr(Reply));
				Response    = XDOC_getRoot(ResponseDoc);
				tb_Free(Reply);

				if(tb_StrEQ(XELT_getName(Response), "methodResponse")) {
					Vector_t Params = XELT_getChildren(tb_Get(XELT_getChildren(Response), 0));

					if(tb_StrEQ(XELT_getName(tb_Get(Params,0)), "fault")) {
						Hash_t Rez = tb_unMarshall(tb_Get(XELT_getChildren(tb_Get(Params,0)), 0));
						int fault = tb_toInt(Rez, "faultCode");
						tb_Free(Rez);
						tb_Free(ResponseDoc);
						tb_Free(Params);
						return fault;

					} else if(tb_getSize(Params) == tb_getSize(outVector)) {
						Iterator_t outIt   = tb_Iterator(outVector);
						Iterator_t paramIt = tb_Iterator(Params);
						
						if(outIt && paramIt) {
							do {
								// check type
								// dig inside <param> -> <value> --> <type>
								XmlElt_t Xe = tb_Get(XELT_getChildren(tb_Get(XELT_getChildren(tb_Value(paramIt)), 0)),0);

								if(tb_valid(tb_Value(outIt), XRpc_getResponseParamType(Xe), __FUNCTION__)) {
									// unMarshall param and affect contents into outVector[n]'s object
									XRpc_unMarshallAndMerge(tb_Value(outIt), Xe);
								} else {
									tb_Free(ResponseDoc);
									tb_Free(outIt);
									tb_Free(paramIt);
									tb_Free(outVector);
									return XMLRPC_FAULT_TYPE_MISMATCH;
								}
							} while(tb_Next(outIt) && tb_Next(paramIt));
							tb_Free(outIt);
							tb_Free(paramIt);
							tb_Free(ResponseDoc);
							tb_Free(outVector);
							return TB_OK;

						} else {
							tb_Free(ResponseDoc);
							tb_Free(outVector);
							if(outIt) tb_Free(outIt);
							if(paramIt) tb_Free(paramIt);
							return XMLRPC_FAULT_BAD_RESPONSE;
						}
					}
				} else {
					tb_Free(outVector);
					return XMLRPC_FAULT_NOT_RESPONDING;
				}
			} else {
				tb_Free(outVector);
				return XMLRPC_FAULT_NOT_RESPONDING;
			}
		} else {
			tb_warn("bad signature\n");
		}
	} 
	return XMLRPC_FAULT_NO_SUCH_CALL;
}

/*
<?xml version="1.0"?>
<methodCall>
   <methodName>myFunc</methodName>
   <params>
      <param>
         <value><int>41</int></value>
      </param>
      <param>
         <value><string>voila</string></value>
      </param>
   </params>
</methodCall>
*/



/**
 * XmlRpc Server-side function
 *
 * This function is spawn on incoming connection on socket So. 
 * @param So incoming RPC connection
 * @see XRpc_senCall, tb_initSockServer 
 * @ingroup Xmlrpc
 */
void XRpc_receiveCall(Socket_t So) {
	XmlRpc_t Xr       = (XmlRpc_t)tb_getServArgs(So);
	String_t Request  = tb_String(NULL);
	XmlDoc_t QueryDoc, Sig;
	XmlElt_t Query;
	Vector_t argVector = NULL;
	String_t methodName;
	String_t Reply = tb_String("%s", "<?xml version=\"1.0\"?>\n<methodResponse>\n");
	int      rc;

	while(tb_readSock(So, Request, MAX_BUFFER)>0);

	fprintf(stderr, "XRpc_receiveCall: <%s>\n", tb_toStr(Request));

	if(tb_getSize(Request) == 0) {
		tb_Free(Request);
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL);
		goto send_reply;
	}

	QueryDoc   = tb_XmlDoc(tb_toStr(Request));
	tb_Free(Request);

	if(QueryDoc == NULL) {
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL);
		goto send_reply;
	}

	if((Query      = XDOC_getRoot(QueryDoc)) == NULL) {
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL);
		goto send_reply;
	}
	if((methodName = XRpc_getQueryMethodName(Query)) == NULL) {
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL);
		goto send_reply;
	}

	Sig        = XRpc_getSignature(Xr, tb_toStr(methodName));

	if(Sig) {
		Iterator_t SigParamIt   = tb_Iterator(XELT_getChildren(XDOC_getRoot(Sig)));
		Vector_t   Params       = XELT_getChildren(tb_Get(XELT_getChildren(Query), 1));
		Iterator_t QueryParamIt = tb_Iterator(Params);
		int rc;
		if(SigParamIt && QueryParamIt &&
			 tb_getSize(SigParamIt) == tb_getSize(QueryParamIt)) {

			argVector = tb_Vector();
			do {
				//<param><value><string>voila</string></value> : must dig from <param> to <string>
				XmlElt_t Xe = tb_Get(XELT_getChildren(tb_Get(XELT_getChildren(tb_Value(QueryParamIt)),0)),0);
				tb_Object_t O = tb_XmlunMarshall(Xe);
				if(tb_valid(O, XRpc_getParamType(tb_Value(SigParamIt)), __FUNCTION__)) {
					tb_Push(argVector, O);
				}
			} while(tb_Next(SigParamIt) && tb_Next(QueryParamIt));
			tb_Free(QueryParamIt);
			// Not freeing SigParamIt at this point 
		}

		tb_warn("will call native\n");

		rc = XRpc_callNative(Xr, tb_toStr(methodName), argVector);

		if(rc == TB_OK) {
			Iterator_t argIt = tb_Iterator(argVector);
			tb_First(SigParamIt);
			if(SigParamIt && argIt) {
				String_t Tmp;
				tb_StrAdd(Reply, -1, "  <params>\n");

				do {
					int style = XRpc_getParamStyle(tb_Value(SigParamIt));
				
					if(style == XMLRPC_PARAM_INOUT || style == XMLRPC_PARAM_OUT) {
						tb_StrAdd(Reply, -1, "    <param><value>%S</value></param>\n", Tmp = tb_Marshall(tb_Value(argIt)));
						tb_Free(Tmp);
					} 

				} while(tb_Next(SigParamIt) && tb_Next(argIt));
				tb_StrAdd(Reply, -1, "  </params>\n</methodResponse>");
				tb_Free(argIt);
				tb_Free(SigParamIt);
				if(argVector) tb_Free(argVector);
			}
		} else {
			XRpc_makeFaultResponse(Reply, rc);
		}
	} else {
		tb_warn("Sig not found\n");
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL);
	}

 send_reply:
	tb_warn("reply<%S>\n", Reply);

	rc = tb_writeSock(So, tb_toStr(Reply));
	tb_warn("send reply rc=%d\n", rc);
	tb_Clear(So);
	tb_Free(Reply);
}


static void XRpc_makeFaultResponse(String_t Response, int fault) {
	tb_Clear(Response);
	tb_StrAdd(Response, -1, "%s", "<?xml version=\"1.0\"?>\n");
	tb_StrAdd(Response, -1,    "<methodResponse>\n");
	tb_StrAdd(Response, -1,    "  <fault>\n");
	tb_StrAdd(Response, -1,    "    <struct>\n");
	tb_StrAdd(Response, -1,    "      <member>\n");
	tb_StrAdd(Response, -1,    "        <name>faultCode</name>\n");
	tb_StrAdd(Response, -1,    "        <value><int>%d</int><value>\n");
	tb_StrAdd(Response, -1,    "      </member>\n");
	tb_StrAdd(Response, -1,    "      <member>\n");
	tb_StrAdd(Response, -1,    "        <name>faultString</name>\n");
	tb_StrAdd(Response, -1,    "        <value><string>remote function error</string><value>\n");
	tb_StrAdd(Response, -1,    "      </member>\n");
	tb_StrAdd(Response, -1,    "    </struct>\n");
	tb_StrAdd(Response, -1,    "  </fault>\n");
	tb_StrAdd(Response, -1,    "</methodResponse>\n");
}


static int XRpc_callNative(XmlRpc_t Xr, char *methodName, Vector_t argVector) {
	int rc = -1;

	switch(tb_getSize(argVector)) {
	case 0:
	case 1: 
		{
			_p1__t _p1 = (_p1__t)XRpc_getMethod(Xr, methodName);
		rc = _p1(tb_Get(argVector,0));
		}
		break;
	case 2:
		{
			_p2__t _p2 = (_p2__t)XRpc_getMethod(Xr, methodName);
			rc = _p2(tb_Get(argVector,0),
							 tb_Get(argVector,1));
		}
		break;
	case 3:
		{
			_p3__t _p3 = (_p3__t)XRpc_getMethod(Xr, methodName);
			rc = _p3(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2));
		}
		break;
	case 4:
		{
			_p4__t _p4 = (_p4__t)XRpc_getMethod(Xr, methodName);
			rc = _p4(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3));
		}
		break;
	case 5:
		{
			_p5__t _p5 = (_p5__t)XRpc_getMethod(Xr, methodName);
			rc = _p5(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4));
		}
		break;
	case 6:
		{
			_p6__t _p6 = (_p6__t)XRpc_getMethod(Xr, methodName);
			rc = _p6(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4),
							 tb_Get(argVector,5));
		}
		break;
	case 7:
		{
			_p7__t _p7 = (_p7__t)XRpc_getMethod(Xr, methodName);
			rc = _p7(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4),
							 tb_Get(argVector,5),
							 tb_Get(argVector,6));
		}
		break;
	case 8:
		{
			_p8__t _p8 = (_p8__t)XRpc_getMethod(Xr, methodName);
			rc = _p8(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4),
							 tb_Get(argVector,5),
							 tb_Get(argVector,6),
							 tb_Get(argVector,7));
		}
		break;
	case 9:
		{
			_p9__t _p9 = (_p9__t)XRpc_getMethod(Xr, methodName);
			rc = _p9(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4),
							 tb_Get(argVector,5),
							 tb_Get(argVector,6),
							 tb_Get(argVector,7),
							 tb_Get(argVector,8));
		}
		break;
	default:
		//fault : too many args
		break;
	}

	return rc;
}









