// $Id: hash_test.c,v 1.2 2004/05/24 16:37:53 plg Exp $

#include <stdio.h>
#include <stdlib.h>
#include "Toolbox.h"

int main(int argc, char **argv) {
	Hash_t H, Z, T;
	String_t S;
	Vector_t V = NULL;
	int i;
	char key[10];
	
//	tb_errorlevel = TB_DEBUG;

	while(1) {
		H = tb_Hash();
		tb_Dump(H);

		Z = tb_HashX(KT_POINTER, 0);


		//		tb_HashFreeze(H);

		tb_profile("create 100 entries :\n");
		for(i = 0; i < 100; i++) {
			sprintf(key, "%04d", i);
			tb_Replace(H, tb_String("test value %04d", i), key);
			S = tb_String("test value %04d", i);
			tb_Replace(Z, S, S);
		}
		tb_profile("done\n");
		//		exit(0);
		tb_Dump(Z);

		//		tb_HashThaw(H);
		//		tb_Dump(H);

		tb_profile("Clone H:\n");

		Z = tb_Clone(H);
		tb_profile("done\n");

		tb_profile("Clear H:\n");
		tb_Clear(H);
		tb_profile("done\n");
		tb_Dump(H);


		tb_trace(TB_WARN, "exists key '0001' in H ? : %d ", tb_Exists(H, "0001"));
		tb_trace(TB_WARN, "exists key '0001' in Z ? : %d ", tb_Exists(Z, "0001"));

		tb_profile("Peek Z{'0002'} :\n");
		tb_Dump(tb_Get(Z, "0002"));
		tb_profile("try to free  Z{'0002'} :\n");
		tb_Free(tb_Get(Z, "0002"));
		tb_Dump(tb_Get(Z, "0002"));
		tb_profile("Get Z{'0002'} :\n");
		tb_Dump(S = tb_Take(Z, "0002"));

		tb_Dump(tb_Get(Z, "0001"));
		tb_trace(TB_WARN, "exists key '0002' in Z ? :%d ", tb_Exists(Z, "0002"));
		tb_Dump(S);
		tb_Remove(Z, "0003");
		tb_trace(TB_WARN, "exists key '0003' in Z ? :%d ", tb_Exists(Z, "0003"));

		V = tb_HashKeys(Z);
		tb_trace(TB_WARN, "Got %d keys", tb_getSize(V));


		//		tb_HashNormalize(Z);
		//		tb_profile("tb_errno = %d\n", tb_errno);



		//-->

		tb_Insert(H, tb_String("ABC"), "abC");
		tb_Insert(H, tb_String("DEF"), "AbC");

		tb_profile("tb_errno = %d\n", tb_errno);

		tb_Replace(H, tb_String("GHI"), "aBc");

		tb_Dump(H);
		tb_Free(H);

		//		tb_HashNormalize(H);

		H = tb_HashX(KT_STRING_I, 0);

		tb_Insert(H, tb_String("ABC"), "abC");

		tb_Free(S);
		if( tb_Insert(H, S = tb_String("DEF"), "AbC") != TB_OK ) {
			tb_Free(S);
		}


		// --


		tb_profile("tb_errno = %d\n", tb_errno);


		tb_Replace(H, tb_String("GHI"), "aBc");

		tb_Dump(H);

		tb_Free(V);


		tb_Clear(H);
		tb_Clear(Z);
		T = tb_Hash();


		//		tb_errorlevel = TB_NOTICE;
		tb_profile("add in H");
		S = tb_String("A");

		tb_Replace(H, S, "first");
		tb_Dump(H);
		tb_profile("add same in Z");
		tb_Replace(Z, tb_Get(H, "first"), "second");
		tb_Dump(Z);
		tb_profile("add same in T");
		tb_Replace(T, tb_Get(H, "first"), "third");
		tb_Dump(T);
		tb_profile("remove from H");
		tb_Remove(H, "first");
		tb_Dump(H);
		tb_Free(H);

		tb_Dump(S);

		tb_profile("free T");
		tb_Free(T);
		tb_Dump(S);
		tb_profile("show Z");
		tb_Dump(Z);

		{
			XmlElt_t X;
			Hash_t W;
			//			tb_errorlevel = TB_DEBUG;
			
			tb_Clear(S);
			tb_profile("marshall --------------------");
			X = tb_Marshall(Z);
			tb_Dump(X);
			tb_profile("unmarshall --------------------");
			W = tb_unMarshall(X);
			tb_Dump(W);
		}


		tb_Free(Z);




#if 1
		break;
#endif
	}
	return 0;
}



		#if 0
		tb_Free(Z);
		tb_Free(H);

		tb_Free(V);
		exit(0);
		#endif


