//=======================================================
// $Id: XmlRpc_util.c,v 1.7 2005/05/12 21:53:10 plg Exp $
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
#include <string.h>
#include <errno.h>

#include "Toolbox.h"
#include "tb_ClassBuilder.h"
#include "XmlRpc.h"
#include "Pointer.h"

static int  XRpc_callNative          (XmlRpc_t Xr, char *methodName, Vector_t argVector);
static void XRpc_makeFaultResponse   (String_t Response, int fault, char *faultstring);


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
int XRpc_sendCall(XmlRpc_t Xr, Socket_t So, char *func, ...) {
	int rc;
	XmlDoc_t Sig;
	
	Sig = XRpc_getSignature(Xr, func);
	if(Sig) {

		Vector_t SigParams = XELT_getChildren(XDOC_getRoot(Sig));
		int i, nb_args     = tb_getSize(SigParams);

			va_list parms;

			XmlDoc_t ResponseDoc = NULL;
			XmlElt_t Response    = NULL;

			Vector_t outVector = tb_Vector();
			String_t Request   = tb_String("%s", "<?xml version=\"1.0\"?>\n<methodCall>\n");
			va_start(parms, func);

			tb_StrAdd(Request, -1, "  <methodName>%S</methodName>\n", XRpc_getMethodName(XDOC_getRoot(Sig)));
			tb_StrAdd(Request, -1, "  <params>\n");
		
		for(i=0; i<nb_args; i++) {

				String_t Tmp;
			XmlElt_t    SigParam    = tb_Get(SigParams, i);
				tb_Object_t O = (tb_Object_t)va_arg(parms, tb_Object_t);
			int         type        = XRpc_getParamType(SigParam);
			int         real_type   = tb_isA(O);
			int         style       = XRpc_getParamStyle(SigParam);

			char *dbg_style[] = {"XMLRPC_PARAM_IN", "XMLRPC_PARAM_INOUT", "XMLRPC_PARAM_OUT"};

			tb_notice("type=%s(real:%s)/style=%s\n", tb_nameOf(type), tb_nameOf(tb_isA(O)), dbg_style[style]);

			switch(style) {
			case XMLRPC_PARAM_IN:
			case XMLRPC_PARAM_INOUT:
				{
					// check if arg type match w/ signature
					// allow TB_POINTER type when signature is <any>
					if((type == TB_OBJECT && real_type == TB_POINTER) || tb_valid(O, type, __FUNCTION__)) {

					// serialise all parameters
						if(type == TB_OBJECT && real_type == TB_POINTER) {
							Tmp = tb_String("<any/>"); // allow late typing
						} else {
					tb_chomp(Tmp = tb_Marshall(O));
						}
					tb_StrAdd(Request, -1, "    <param><value>%S</value></param>\n", Tmp);
					tb_Free(Tmp);

				} else {
						tb_error("arg type mismatch\n");
					tb_Free(Request);
					return XMLRPC_FAULT_TYPE_MISMATCH;
				}

				} break;
			case XMLRPC_PARAM_OUT: // param out are not sent
				{
				} break;
			}	
		}
		
			tb_StrAdd(Request, -1, "  </params>\n");
			tb_StrAdd(Request, -1, "</methodCall>\n");
			
			if(tb_errorlevel >TB_WARN) {fprintf(stderr, "%s", tb_toStr(Request));}

			rc = tb_writeSock(So, tb_toStr(Request));
			tb_Free(Request);

			if(rc != TB_ERR && rc != TB_KO) {
				String_t Reply = tb_String(NULL);
			int retcode;
				
				shutdown(tb_getSockFD(So), SHUT_WR); // cut write uplink
			while((retcode = tb_readSock(So, Reply, MAX_BUFFER)) >0);
			if(retcode <0) {
				tb_warn("Xrpc_sendCall: read reply failed (%s)\n", strerror(errno));
			}
				ResponseDoc = tb_XmlDoc(tb_toStr(Reply));
				if(ResponseDoc != NULL) {
					Response    = XDOC_getRoot(ResponseDoc);
				}

			if(tb_errorlevel >TB_WARN) {
				fprintf(stderr, "XRpc_sendCall: got reply<\n%s\n>\n", tb_toStr(Reply)	); 
				tb_hexdump(tb_toStr(Reply), tb_getSize(Reply));
			}

				tb_Free(Reply);

				if(Response && tb_StrEQ(XELT_getName(Response), "methodResponse")) {
				Vector_t Params = XELT_getChildren(XELT_getFirstChild(Response));

				if(tb_StrEQ(XELT_getName(XELT_getFirstChild(Response)), "fault")) {
					Hash_t Rez = tb_XmlunMarshall(tb_Get(Params, 0));
						int fault = tb_toInt(Rez, "faultCode");
						tb_Free(Rez);
						tb_Free(ResponseDoc);
					tb_warn("got a fault response\n");

						return fault;

				} else if(tb_StrEQ(XELT_getName(XELT_getFirstChild(Response)), "params")) {

					Vector_t Values =  XELT_getChildren(XELT_getFirstChild(Response));
					Iterator_t It   = tb_Iterator(Values);

					va_start(parms, func);

					for(i=0; i<nb_args; i++) {

						XmlElt_t    SigParam    = tb_Get(SigParams, i);
						int         style       = XRpc_getParamStyle(SigParam);
						tb_Object_t Arg         = (tb_Object_t)va_arg(parms, tb_Object_t);

						switch(style) {
						case XMLRPC_PARAM_IN: // <in> params are not sent back
							tb_notice("arg[%d] IN: skipped\n", i);
							break;
						case XMLRPC_PARAM_INOUT:
						case XMLRPC_PARAM_OUT:
							{

								tb_notice("arg[%d] OUT/INOUT: need merge (%s)\n", i, tb_nameOf(tb_isA(Arg)));

								XmlElt_t    Xe  = XELT_getFirstChild(XELT_getFirstChild(tb_Value(It)));

								if(tb_isA(Arg) == TB_POINTER) { // requiered type == 'any'
									tb_Object_t O = tb_XmlunMarshall(Xe);
									Pointer_ctor(Arg, O, NULL);
								} else if(tb_valid(Arg, XRpc_getResponseParamType(Xe), __FUNCTION__)) {
									// unMarshall param and affect contents into arg[n]
									XRpc_unMarshallAndMerge(Arg, Xe);
								} else {
									tb_Free(ResponseDoc);
									tb_Free(It);
									tb_Free(outVector);
									return XMLRPC_FAULT_TYPE_MISMATCH;
								}
								tb_Next(It);
							} break;
 
						}

						tb_Free(It);
							tb_Free(ResponseDoc);
							return TB_OK;

					}

						} else {
							tb_Free(outVector);
					return XMLRPC_FAULT_BAD_RESPONSE; // invalid number of args
					}
				} else {
					tb_Free(outVector);
					return XMLRPC_FAULT_BAD_RESPONSE; // invalid number of args
				}
			} else {
				return XMLRPC_FAULT_NOT_RESPONDING;
			}
		} else {
			tb_warn("bad signature\n");
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

	if(tb_errorlevel >TB_WARN) {fprintf(stderr, "XRpc_receiveCall: <%s>\n", tb_toStr(Request)); }

	if(tb_getSize(Request) == 0) {
		tb_Free(Request);
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL, "No such method");
		goto send_reply;
	}

	QueryDoc   = tb_XmlDoc(tb_toStr(Request));
	tb_Free(Request);

	if(QueryDoc == NULL) {
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL, "No such method");
		goto send_reply;
	}

	if((Query      = XDOC_getRoot(QueryDoc)) == NULL) {
		tb_Free(QueryDoc);
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL, "No such method");
		goto send_reply;
	}
	if((methodName = XRpc_getQueryMethodName(Query)) == NULL) {
		tb_Free(QueryDoc);
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL, "No such method");
		goto send_reply;
	}

	Sig        = XRpc_getSignature(Xr, tb_toStr(methodName));

/* <methodCall> */
/*   <methodName>facade.getValue</methodName> */
/*   <params> */
/*     <param><value><int>5</int></value></param> */
/*     <param><value><int>6</int></value></param> */
/*     <param><value><string>sys.net.ip.route.Routes2</string></value></param> */
/*     <param><value><any/></value></param> */
/*     <param><value><int/></value></param> */
/*   </params> */
/* </methodCall> */

	if(Sig) {

			argVector = tb_Vector();

		Vector_t   SigParams     = XELT_getChildren(XDOC_getRoot(Sig));
		int        i, nb_args    = tb_getSize(SigParams);
		Vector_t   QueryParams   = XELT_getChildren(tb_Get(XELT_getChildren(Query), 1));
		Iterator_t QueryParamsIt = tb_Iterator(QueryParams);

		
		for(i=0;i<nb_args; i++) {

			XmlElt_t    SigParam    = tb_Get(SigParams, i);
			int         type        = XRpc_getParamType(SigParam);
			int         mode        = XRpc_getParamStyle(SigParam);
			tb_Object_t O;

			tb_notice("arg[%d] : %s : %d\n", i, tb_nameOf(type), mode);

			switch(mode) {
			case XMLRPC_PARAM_IN:
			case XMLRPC_PARAM_INOUT:
				{
					XmlElt_t    Xe = XELT_getFirstChild(XELT_getFirstChild(tb_Value(QueryParamsIt)));

					if(type == TB_STRING) { // <string> </string> tags are optionnals :<
						XmlElt_t Xe2 = XELT_getFirstChild(Xe);
						if(Xe2 && XELT_getType(Xe2) == XELT_TYPE_NODE && XELT_getName(Xe2) != NULL) {
							if(tb_StrEQ(XELT_getName(Xe2), "string")) {
								O = tb_XmlunMarshall(Xe2);
							} else {
								O = XELT_getText(XELT_getFirstChild(Xe));
							}
						} else {
							O = XELT_getText(XELT_getFirstChild(Xe));
						}
					} else {
						O = tb_XmlunMarshall(Xe);
					}

					if(tb_valid(O, type, __FUNCTION__)) {
					tb_Push(argVector, O);
						tb_notice("pushed arg %s in argVector\n", tb_nameOf(tb_isA(O)));
					} else {
						tb_error("invalid parameter type (waiting for a %s)\n", tb_nameOf(type));
						tb_Free(O);
				}

				} break;
			case XMLRPC_PARAM_OUT: // out parms are not transmitted
				{
					void *p = __getMethod(type, OM_NEW);
					tb_Object_t Tmp = NULL;
					if(p != NULL) {
						Tmp = ((tb_Object_t (*)())p)();
		}
					tb_Push(argVector, Tmp);
					tb_notice("created a new %s in argVector\n", tb_nameOf(type));
				}
			}

		}


		tb_info("will call native\n");

		rc = XRpc_callNative(Xr, tb_toStr(methodName), argVector);

		tb_info("native rc = %d\n", rc);

		if(rc == TB_OK) {
			Iterator_t argIt = tb_Iterator(argVector);

			if(argIt) {
				String_t Tmp;
				tb_StrAdd(Reply, -1, "  <params>\n");

				for(i=0; i<nb_args; i++) {
					XmlElt_t    SigParam    = tb_Get(SigParams, i);
					int         type        = XRpc_getParamType(SigParam);
					int         mode        = XRpc_getParamStyle(SigParam);

					switch(mode) {
					case XMLRPC_PARAM_OUT:
					case XMLRPC_PARAM_INOUT:
						{
							if(type == TB_OBJECT) {
								tb_StrAdd(Reply, -1, "    <param><value>%S</value></param>\n", 
													Tmp = tb_Marshall(P2p(tb_Value(argIt))));
							} else {
						tb_StrAdd(Reply, -1, "    <param><value>%S</value></param>\n", 
											Tmp = tb_Marshall(tb_Value(argIt)));
							}
						tb_Free(Tmp);
							tb_Next(argIt);
						} break;
					case XMLRPC_PARAM_IN:
						tb_Next(argIt);
						break;
					}

					} 

				tb_StrAdd(Reply, -1, "  </params>\n</methodResponse>");
				tb_Free(argIt);
				if(argVector) tb_Free(argVector);
			}
		} else {
			XRpc_makeFaultResponse(Reply, rc, "remote procedure error code");
			if(argVector) tb_Free(argVector);
		}
	} else {
		tb_warn("Sig not found\n");
		XRpc_makeFaultResponse(Reply, XMLRPC_FAULT_NO_SUCH_CALL, "no such method");
	}
	tb_Free(QueryDoc);

 send_reply:
	if(tb_errorlevel > TB_WARN) {
		fprintf(stderr, "XmlRPC Reply: \n%s\n", tb_toStr(Reply));
	}


	rc = tb_writeSock(So, tb_toStr(Reply));
	tb_info("send reply rc=%d\n", rc);
	tb_Clear(So);
	tb_Free(Reply);
}


static void XRpc_makeFaultResponse(String_t Response, int fault, char *faultstring) {
	tb_Clear(Response);
	tb_StrAdd(Response, -1, "%s", "<?xml version=\"1.0\"?>\n");
	tb_StrAdd(Response, -1,    "<methodResponse>\n");
	tb_StrAdd(Response, -1,    "  <fault>\n");
	tb_StrAdd(Response, -1,    "    <struct>\n");
	tb_StrAdd(Response, -1,    "      <member>\n");
	tb_StrAdd(Response, -1,    "        <name>faultCode</name>\n");
	tb_StrAdd(Response, -1,    "        <value><int>%d</int></value>\n", fault);
	tb_StrAdd(Response, -1,    "      </member>\n");
	tb_StrAdd(Response, -1,    "      <member>\n");
	tb_StrAdd(Response, -1,    "        <name>faultString</name>\n");
	tb_StrAdd(Response, -1,    "        <value><string>%s</string></value>\n", faultstring);
	tb_StrAdd(Response, -1,    "      </member>\n");
	tb_StrAdd(Response, -1,    "    </struct>\n");
	tb_StrAdd(Response, -1,    "  </fault>\n");
	tb_StrAdd(Response, -1,    "</methodResponse>\n");
}


static int XRpc_callNative(XmlRpc_t Xr, char *methodName, Vector_t argVector) {
	int rc = -1;
	void *p = XRpc_getMethod(Xr, methodName);
	
	tb_notice("native <%s/%p> get called with %d args\n", 
						methodName, 
						p,
						tb_getSize(argVector));

	switch(tb_getSize(argVector)) {
	case 0:
	case 1: 
		{
			_p1__t _p1 = (_p1__t)p;
		rc = _p1(tb_Get(argVector,0));
		}
		break;
	case 2:
		{
			_p2__t _p2 = (_p2__t)p;
			rc = _p2(tb_Get(argVector,0),
							 tb_Get(argVector,1));
		}
		break;
	case 3:
		{
			_p3__t _p3 = (_p3__t)p;
			rc = _p3(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2));
		}
		break;
	case 4:
		{
			_p4__t _p4 = (_p4__t)p;
			rc = _p4(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3));
		}
		break;
	case 5:
		{
			_p5__t _p5 = (_p5__t)p;
			rc = _p5(tb_Get(argVector,0),
							 tb_Get(argVector,1),
							 tb_Get(argVector,2),
							 tb_Get(argVector,3),
							 tb_Get(argVector,4));
		}
		break;
	case 6:
		{
			_p6__t _p6 = (_p6__t)p;
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
			_p7__t _p7 = (_p7__t)p;
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
			_p8__t _p8 = (_p8__t)p;
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
			_p9__t _p9 = (_p9__t)p;
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









