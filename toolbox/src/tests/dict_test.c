#undef __BUILD
//#define TB_MEM_DEBUG
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "Toolbox.h"

#include "bplus_tree.h"

Vector_t slurpFile_and_make_array(char *file);


#define BADVAL (tb_Key_t)-1

int main(int argc, char **argv) {
	tb_Object_t O;
	//	Dict_t D = tb_Dict(KT_STRING, 1);
	Dict_t NDict = tb_Dict(KT_INT, 0);
	Hash_t D = tb_HashX(KT_STRING, 1);
	FILE *F;
	char *s, buff[51];
	Vector_t V = tb_Vector();
	Vector_t W = tb_Vector();
	int max, n = 0;
	dictIter_t Di;
	char *k;
	int i=0;

	tb_profile("start: KT_INT:%d KT_STRING:%d\n", KT_INT, KT_STRING);


	tb_warn("Hash: %d", TB_HASH);
	tb_warn("Vector: %d", TB_VECTOR);
	tb_warn("Dict: %d", TB_DICT);
	tb_warn("String: %d", TB_STRING);
	tb_warn("Raw: %d", TB_RAW);
	tb_warn("Num: %d", TB_NUM);
	tb_warn("Pointer: %d", TB_POINTER);

	tb_Insert(D, tb_String("found"), "test");
	tb_Dump(tb_Get(D, "test"));
	fprintf(stderr, "exists ? : %d\n", tb_Exists(D, "test"));
					
	if((F = fopen("duplicates.txt", "r")) == NULL) {
		//	if((F = fopen("dico.txt", "r")) == NULL) {
		//if((F = fopen("short.txt", "r")) == NULL) {
		abort();
	}
	tb_profile("read dico\n");
	while((s = fgets(&buff, 50, F))) {
		String_t S;
		s[strlen(s) -1] = 0;
		S = tb_String("%s", s);
		tb_Push(V, S);
		if( n == 1 ) {
			n=0; 
			tb_Push(W, S);
		} else {
			n=1;
		}
		if( i++ >10000) break;
	}
	tb_profile("done\n");
	max = tb_getSize(V);
	tb_profile("now insert\n");
	for(n=0; n<max; n++) {
		s = tb_toStr(V, n);
		if( strlen(s) >2 ) {
			//			if( tb_Insert(D, tb_String("%s:%d", s, n), s) == 0) {
			if( tb_Insert(D, tb_String("%d", n), s) == 0) {
				fprintf(stderr, "*** %s !\n", s);
		}
	}
	}
	tb_profile("done (size=%d/ nb keys=%d)\n", tb_getSize(D), tb_Dict_numKeys(D));
	max = tb_getSize(W);

	tb_Dump(D);


	if(1) {
		int nb;
		tb_profile("how many 'the' : %d\n", nb=tb_Exists(D, "the"));
		for(i=0; i<nb/2; i++) {
			String_t S;
			tb_warn("%d:  <%S>\n", i, S=tb_Take(D, "the"));
			tb_Free(S);
			tb_profile("how many 'the' : %d\n", tb_Exists(D, "the"));
		}
		tb_Replace(D, tb_String("tutu"), "the");
		tb_profile("how many 'the' : %d\n", tb_Exists(D, "the"));
		tb_Replace(D, tb_String("tutu"), "the");
		tb_profile("how many 'the' : %d\n", tb_Exists(D, "the"));
		tb_Insert(D, tb_String("tutu2"), "the");
		tb_profile("how many 'the' : %d\n", tb_Exists(D, "the"));
		tb_Remove(D, "the");
		tb_profile("how many 'the' : %d\n", tb_Exists(D, "the"));
	}





	tb_profile("now remove half\n");
	for(n=0; n<max; n++) {
		s = tb_toStr(W, n);
		tb_profile("remove <%s>\n", s);
		tb_Remove(D, s);
	}
	tb_profile("done (size=%d)\n", tb_getSize(D));
	//	tb_profile("now clear\n");
	//	tb_Clear(D);
	//	tb_profile("done\n");
	tb_Dump(D);


	//	tb_Dump(tb_Get(D, "zo7Ap26b"));

	//	tb_errorlevel = TB_DEBUG;

 
	if(1) {
		Iterator_t It  = tb_Iterator(D);
		Iterator_t Rev = tb_Iterator(D);
		int i;
		tb_First(It);
		tb_Last(Rev);
		tb_profile("bi-iterate\n");
		for (i=0; i<500; i++) {
			tb_profile("%d)>>> k[%s] ==> v[%S]\n", i, tb_Key(It), tb_Value(It));
			tb_profile("%d)<<< k[%s] ==> v[%S]\n", i, tb_Key(Rev), tb_Value(Rev));

			tb_Insert(NDict, tb_String("%s", tb_Key(It)), tb_toInt(tb_Value(It)));

			if(tb_Next(It) == NULL) break;
			if(tb_Prev(Rev) == NULL) break;
		}

		tb_profile("done\n");
		tb_Free(It);
		tb_Free(Rev);
		
		It = tb_Iterator(NDict);
		tb_First(It); 
		i = 0;
		do {
			tb_profile("%d)=>=>=> k[%d] ==> v[%S]\n", i++, tb_Key(It), tb_Value(It));
		} while(tb_Next(It));
		tb_Free(It);
	}


	tb_Dump(D);


	if(TB_VALID(D, TB_DICT)) {
		char *key = "name";
		Iterator_t It = tb_Dict_upperbound(D, (tb_Key_t)key);
		if( It) {
			tb_profile("find greatest lower or equal <%s>\n", key);
			for(i=0; i<10; i++) {
				tb_profile("ub) k[%s] ==> v[%S]\n", tb_Key(It), tb_Value(It));
				tb_Prev(It);
			}
			tb_Free(It);
		} else {
			tb_profile("can't find greatest lower or equal <%s>\n", key);
		}
	}


	if(TB_VALID(D, TB_DICT)) {
		char *key = "yourz";
		Iterator_t It = tb_Dict_lowerbound(D, (tb_Key_t)key);
		if( It ) {
			tb_profile("find lowest bigger or equal <%s>\n", key);
			for(i=0; i<10; i++) {
				tb_profile("lb) k[%s] ==> v[%S]\n", tb_Key(It), tb_Value(It));
				tb_Next(It);
			}
			tb_Free(It);
		} else {
			tb_profile("can't find lowest bigger or equal <%s>\n", key);
		}
	}


	if(TB_VALID(D, TB_DICT)) {
		Iterator_t It;
		int val = 423;
		if( (It = tb_Dict_upperbound(NDict, (tb_Key_t)val))) {
			tb_profile("find greatest lower or equal <%d>\n", val);
			for(i=0; i<10; i++) {
				tb_profile("lb) k[%d] ==> v[%S]\n", tb_Key(It), tb_Value(It));
				tb_Prev(It);
			}
			tb_Free(It);
		} else {
			tb_profile("can't find greatest lower or equal <%d>\n", val);
		}

		It = tb_Dict_lowerbound(NDict, (tb_Key_t)val);
		if( It ) {
			tb_profile("find lowest bigger or equal <%d>\n", val);
			for(i=0; i<10; i++) {
				tb_profile("lb) k[%d] ==> v[%S]\n", tb_Key(It), tb_Value(It));
				tb_Next(It);
			}
			tb_Free(It);
		} else {
			tb_profile("can't find lowest bigger or equal <%d>\n", val);
		}

	}


	//	tb_Dump(D);
	//	tb_Clear(D);
	tb_profile("free vector\n");
	//	tb_errorlevel = TB_DEBUG;

	tb_Free(W);
	tb_Free(V);
	tb_profile("free dict\n");
	tb_Free(D);
	tb_Free(NDict);
	tb_profile("last fm_dump\n");
	fm_Dump();

	return 0;

}



Vector_t slurpFile_and_make_array(char *file) {
	int rc=1, fd;
	String_t S = NULL;
	char buff[4096];
	Vector_t V = tb_Vector();

	tb_profile(" ------------------------ test file : %s ------\n", file);

	assert(file != NULL);
	assert((fd = open(file, O_RDONLY)) != -1);
	S = tb_String(NULL);
	while( rc > 0) {
		rc = read(fd, buff, 4095);
		if(rc >0 ) {
			buff[rc] = 0;
			tb_StrAdd(S, -1, "%s", buff);
		}
	}
	assert(rc != -1);
	//	fprintf(stderr, "%s", S2sz(S));
	tb_profile("file loaded\n");
	rc = tb_tokenize(V, S2sz(S), "\n", 0);
	tb_profile("%d tokens found\n", rc);
	tb_Free(S);

	return V;
}



#if 0 // test values from 'dico' file
PacFeb
nekbovib
owmocsIf
coogorr)
%OtZajEj
wrowmUs
Vafyeub
Imsaxbo
ocHybr1
UffofUk
Whoihiaf
Jofijken
#endif
