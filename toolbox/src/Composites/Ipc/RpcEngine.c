//=======================================================
// $Id: RpcEngine.c,v 1.1 2005/05/12 21:50:48 plg Exp $
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

#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

#include "Toolbox.h"
#include "Memory.h"
#include "Error.h"
#include "Objects.h"
#include "Pointer.h"
#include "tb_ClassBuilder.h"
#include "Serialisable_interface.h"
#include "Tlv.h"
#include "Rpc.h"

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



static int Rpc_callNative(void *p, Vector_t argVector);
static void dump_frame(char *s, int len);

Raw_t mkFrame(void *data) {
	Raw_t R;
	int   len = Tlv_getFullLen(data);
	void *curr, *frame = tb_xmalloc(len + 6);
	curr = frame;
	*(((char *)curr)++) = 0x02;
	*(((int*)curr)++)   = len;
	memcpy(curr, data, len);
	((char *)frame)[len+5] = 0x03;

	R = tb_Raw(len + 6, frame);
	tb_xfree(frame);

	if(tb_errorlevel >TB_WARN) {
		tb_warn("mkFrame:\n");
		tb_hexdump(Raw_getData(R), tb_getSize(R));
		dump_frame(Raw_getData(R), tb_getSize(R));
	}

	return R;
}


static void dump_frame(char *s, int len) {
	if(len >20) {
		int i, nb;

		fprintf(stderr, "tlv: %02X    : STX\n",    *(s++));
		fprintf(stderr, "tlv:   %08X  : FULL_LEN\n",   *(((int*)s)++));

		fprintf(stderr, "tlv:   %08X  : TYPE\n",   *(((int*)s)++));
		fprintf(stderr, "tlv:   %08X  : LEN\n",    *(((int*)s)++));
		fprintf(stderr, "tlv:   %08X  : MId\n",    *(((int*)s)++));
		fprintf(stderr, "tlv:   %08X  : ElmtNb\n", nb = *(((int*)s)++));
	
		for(i=0; i<nb; i++) {
			int len;
			int type =  *(((int*)s)++);
			fprintf(stderr, "tlv:   #%d  %08X  : TYPE (%s)\n", i, type, tb_nameOf(type));
			fprintf(stderr, "tlv:   #%d  %08X  : LEN\n",  i, len = *(((int*)s)++));
			s+=len;
		}
	
		fprintf(stderr, "tlv: %02X    : ETX\n",    *s);
	}
}


Tlv_t Tlv_add(void *frame, Tlv_t T) {
	if(T && frame) {
		frame = tb_xrealloc(frame, Tlv_getFullLen(frame)  + Tlv_getFullLen(T));
		memcpy(frame+(Tlv_getFullLen(frame)), T,  Tlv_getLen(T)+8);
		*(((int*)frame)+1) += Tlv_getFullLen(T);
		(*(((int*)frame)+3)) ++;
		tb_xfree(T);
	}
	return frame;
}


Tlv_t Tlv_getNext(void *curr) {
	return ((char *)curr += Tlv_getFullLen((Tlv_t)curr));
}


void * chkFrame(String_t Data) {

	if(tb_getSize(Data) == 0) {
		tb_warn("chkFrame: empty frame !\n");
		return NULL;
	}

	char *raw = tb_toStr(Data);
	int len;
	int magic;
	if(tb_errorlevel >TB_WARN) {
		tb_warn("chkFrame:\n");
		tb_hexdump(tb_toStr(Data), tb_getSize(Data));
		dump_frame(tb_toStr(Data), tb_getSize(Data));
	}

	if(*(raw++) != 0x02) {
		tb_error("chkFrame: bad framing (missing etx)\n");
		return NULL;
	}
	len = *(((int*)raw)++);
	if(len > (tb_getSize(Data))){
		tb_error("chkFrame: bad size (%d)\n", len);
		return NULL;
	}

	if(raw[len] != 0x03) {
		tb_error("chkFrame: bad framing (missing stx) \n");
		return NULL;
	}
	magic = *(((int*)raw)++); 
	switch(magic) {
	case RPC_REQUEST:
		break;
	case RPC_REPLY:
		break;
	default:
		tb_error("chkFrame: bad RPC_METHOD identifier (%d)\n", magic);
		return NULL;
	}

	((int*)raw)++;
	if(tb_errorlevel >TB_WARN) tb_hexdump(raw, len-8);

	return (void *)raw;
}

int Rpc_sendCall(Rpc_t Rpc, Socket_t So, int funcId, ...) {
	void    * tlv_data, *reply_frame;
	Raw_t     Frame;
	int       rc, retcode, nbargs;
	rpc_sig_t sig = Rpc_getMethod(Rpc, funcId);

	if(sig) {
		va_list parms;
		int i;
		tlv_data= tb_xmalloc(16);
		*(int*)tlv_data       = RPC_REQUEST;        // Type
		*(((int*)tlv_data)+1) = 8;                  // len
		*(((int*)tlv_data)+2) = funcId;             // value.method_id
		*(((int*)tlv_data)+3) = 0;                  // value.nb_elemts

		va_start(parms, funcId);
		for(i=0; i<sig->nb_args; i++) {
			tb_Object_t O = (tb_Object_t)va_arg(parms, tb_Object_t);
			if(! TB_VALID(O, sig->Args[i].type)) {
				tb_error("rpc arg %d type mismatch (%s should be %s)\n", 
								 i, tb_nameOf(tb_isA(O)), tb_nameOf(sig->Args[i].type));

				tb_xfree(tlv_data);
				return RPC_MISMATCHING_TYPES;
			}
			switch(sig->Args[i].mode) {
			case RPC_IN:
				{
					tlv_data = Tlv_add(tlv_data, tb_toTlv(O));
				} break;
			case RPC_INOUT:
				{
					tlv_data = Tlv_add(tlv_data, tb_toTlv(O));
				} break;
			case RPC_OUT:
				// out parameters are not sent
				break;
			}
		}


		Frame = mkFrame(tlv_data);
		tb_xfree(tlv_data);

		rc = tb_writeSockBin(So, Frame);
		shutdown(tb_getSockFD(So), SHUT_WR); // cut write uplink

		tb_Free(Frame);

		if(rc == TB_ERR || rc == TB_KO) {
			tb_error("rpc transport error : can't contact remote server\n"); 
			return RPC_TRANSPORT_ERROR;
		}

		String_t Reply = tb_String(NULL);
			

		while(tb_readSock(So, Reply, MAX_BUFFER) >0);
		if(tb_getSockStatus(So) != TB_CONNECTED) {
			char *sock_status[] ={ "BROKEN", "UNSET", "DISCONNECTED", "CONNECTED", "LISTENING", "TIMEOUT" };

			tb_error("rpc reply failed (sock status: %s)\n", sock_status[tb_getSockStatus(So)]);
			tb_Free(Reply);
			return RPC_REPLY_CORRUPTED;
		}

		if((reply_frame = chkFrame(Reply)) == NULL) {
			tb_error("rpc reply corrupted/empty (req was %d)\n", funcId);
			tb_Free(Reply);
			return RPC_REPLY_CORRUPTED;
		}

		retcode = *(((int*)reply_frame)++);
		nbargs  = *(((int*)reply_frame)++);

		if(retcode == TB_OK) {
			void *curr = reply_frame;
			va_start(parms, funcId);
			for(i=0; i<sig->nb_args; i++) {
				tb_Object_t O = (tb_Object_t)va_arg(parms, tb_Object_t);
				switch(sig->Args[i].mode) {
				case RPC_IN:
					// in parameters are not sent back
					break;
				case RPC_INOUT:
				case RPC_OUT:
					{
						tb_Object_t tmp = tb_fromTlv(curr);
						if(tb_isA(O) == TB_POINTER) {
							Pointer_ctor(O, tmp, NULL);
						} else {
							tb_Set(O, tmp);
							tb_Free(tmp);
						}
						curr = Tlv_getNext(curr);
					}
				}
			}
			tb_Free(Reply);
			return RPC_SUCCESS;
		} else {
			tb_Free(Reply);
			return RPC_REPLY_ERROR;
		}			
	}
	return RPC_UNKNOWN_METHOD;
}



void Rpc_receiveCall(Socket_t So) {
	Rpc_t      Rpc     = (Rpc_t)tb_getServArgs(So);
	String_t   Request = tb_String(NULL);
	Raw_t      Frame;
	void     * curr;
	int        i, rc, signum, nbargs;
	rpc_sig_t  sig;
	Vector_t   args;
	char     * data, *tlv_data;


	while((rc = tb_readSock(So, Request, MAX_BUFFER)) >0);

	if((data = chkFrame(Request)) == NULL) {
		tb_error("Rpc_receiveCall: rpc request corrupted\n");
		tb_Free(Request);
		return;
	}
	
	signum = *(((int*)data)++);

	if((sig = Rpc_getMethod(Rpc, signum)) == NULL) {
		tb_error("Rpc_receiveCall: no such method (%d)\n", signum);
		tb_Free(Request);
		return;
	}

	nbargs = *(((int*)data)++);

	args = tb_Vector();
	curr = data;

	for(i=0; i<sig->nb_args; i++) {
		Tlv_t T = curr;
		//		tb_warn("Rpc_receiveCall: arg#%d %p <%s>\n", i, T, tb_nameOf(Tlv_getType(T)));
		switch(sig->Args[i].mode) {
		case RPC_IN:
		case RPC_INOUT:
			tb_Push(args, tb_fromTlv(T));
			//			tb_warn("Rpc_receiveCall: arg#%d %p <%s> pushed ok\n", i, T, tb_nameOf(Tlv_getType(T)));
			curr = Tlv_getNext(curr);
			break;
		case RPC_OUT: // out parameters are not transmitted, set up temp objects for native call return values
			{

				int type = (sig->Args[i].type == TB_OBJECT) ? TB_POINTER : sig->Args[i].type;
				void *p = __getMethod(type, OM_NEW);
				tb_Object_t Tmp = NULL;
				if(p != NULL) {
					Tmp = ((tb_Object_t (*)())p)();
				}
				tb_Push(args, Tmp);
			}
		}
	}
	
	tb_Free(Request);
	rc = Rpc_callNative(sig->functor, args);

	tlv_data = tb_xmalloc(16);
	*(int*)tlv_data = RPC_REPLY;          // Type
	*(((int*)tlv_data)+1) = 8;            // len
	*(((int*)tlv_data)+2) = rc;           // value.return_code
	*(((int*)tlv_data)+3) = 0;            // value.nb_elemts

	for(i=0; i<sig->nb_args; i++) {
		switch(sig->Args[i].mode) {
		case RPC_IN: // in parameters are not sent back
			break;
		case RPC_INOUT:
		case RPC_OUT:
			{
				tb_Object_t O = tb_Get(args, i);
				if(sig->Args[i].type == TB_OBJECT) { // means *any* object type, had been encaps in tb_Pointed
					tb_Object_t T = P2p(O);            // de-encaps real value
					tb_Free(O);
					O = T;
					tlv_data = Tlv_add(tlv_data, tb_toTlv(O));
					tb_Free(O);
				} else {
					tlv_data = Tlv_add(tlv_data, tb_toTlv(O));
				}
			}
		}
	} 
	tb_Free(args);

	Frame = mkFrame(tlv_data);
	tb_xfree(tlv_data);
	if(tb_errorlevel >TB_WARN) tb_hexdump(Raw_getData(Frame), tb_getSize(Frame));
	tb_writeSockBin(So, Frame);
	tb_Clear(So);
	tb_Free(Frame);
}




static int Rpc_callNative(void *p, Vector_t argVector) {
	int rc = -1;

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















#if 0
int myFunc() { return 0; }
#define MY_FUNC_ID 9999

int test() {
	
	struct rpc_sig sig = { myFunc, 
												 MY_FUNC_ID, 
												 3,
												 {
													 {TB_STRING, RPC_IN},
													 {TB_STRING, RPC_OUT},
													 {TB_NUM,    RPC_OUT},
												 }
	};

}
#endif
