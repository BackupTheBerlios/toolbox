// $Id: vector_test.c,v 1.1 2004/05/12 22:05:15 plg Exp $

#define TB_MEM_DEBUG

#include <stdio.h>
#include <stdlib.h>
#include "Toolbox.h"



void my_trace(char *arg) {
	fprintf(stderr, "--->my trace---> [%s]\n", arg);
}

char *getmyname(void) {
	char *s;
	tb_asprintf(&s, "poil");
	return s;
}

int main(int argc, char **argv) {
	Vector_t V, W;
	String_t S, T;

	void *p;

	char str[] = "10803551 10817246 12120887 13158625 13158627 13158628 13158630 13158632 13158633 13158634 13158635 13158637";

	char str2[] = "9175671 9205348 9261196 9327975 9328060 9330988 9331049 9334260 9387462 9387554 9439690 9440498 9570101 9570134 10086719 10103019 10465797 10466474 10466665 10584336 10586793 10600940 10631836 10634039 10654429 10827350 10829669 10838160 11139404 11142142 11178380 11178385 11475166 11475684 11507366 13070161 13073652 13089439 13101587";

	setenv("TB_MEM_DEBUG", "vector.dbg", 1);
	//	tb_memDebug();

	/*
	tb_setGetNameFnc(getmyname);
	tb_setTraceFnc(my_trace);

	tb_errorlevel = TB_DEBUG;
	*/


	V = tb_Vector();
	tb_tokenize(V, str, " ", 0);
	tb_Dump(V);
	while(tb_getSize(V)) {
		tb_Object_t O = tb_Pop(V);
		tb_Free(O);
		//		tb_Dump(V);
	}
	//	tb_Dump(V);
	tb_Clear(V);
	tb_tokenize(V, str2, " ", 0);
	tb_Dump(V);
	while(tb_getSize(V)) {
		tb_Object_t O = tb_Pop(V);
		tb_Free(O);
		tb_Dump(V);
	}
	tb_Dump(V);

	tb_Free(V);

	V = tb_Vector();

	S = tb_String("%s", "1/2/3/4/5/6/7/8/9");
	tb_Dump(S);
	tb_warn("tb_tokenize rc = %d\n", 
					 tb_tokenize(V, tb_toStr(S), "/", 0));
	tb_Dump(V);




	{
		Vector_t W;
		//		tb_errorlevel = TB_DEBUG;

		tb_Free(S);
		S = tb_Marshall(V);
		fprintf(stderr, "-->\n%s\n<--\n", S2sz(S));
		W = tb_unMarshall(S);
		tb_Dump(W);
	}



	tb_StrAdd(tb_Clear(S), 0, "%s", "Blip"); 

	T = tb_String("%s", "un deux 'trois quatre' ' cinq"); 

	tb_Clear(V);
	tb_Dump(V);
	tb_tokenize(V, tb_toStr(S), "/", 0);
	tb_Dump(V);


	fprintf(stderr, "#0: [%s]\n", tb_toStr(V,0));


	W = tb_Vector();
	tb_tokenize(W, tb_toStr(T), " ", TK_ESC_QUOTES);
	tb_Dump(W);

	//	exit(0);

	tb_Replace(W, S, 5);
	tb_Replace(W, S, 2);
	tb_Dump(W);
	tb_Dump(tb_Pop(W));
	tb_Dump(W);
	tb_Dump(tb_Shift(W));
	tb_Dump(W);
	tb_StrAdd(tb_Clear(T), 0, "%s", "Bloup"); 
	tb_Unshift(W, T);
	tb_Dump(W);
	tb_Free(V);
	V = tb_Clone(W);
	tb_Dump(V);
	tb_Dump(tb_Sort(V, NULL));
	tb_Dump(tb_Reverse(V));
	tb_Free(tb_Get(V, 1));
	tb_Dump(V);
	tb_Remove(V, 0);
	tb_Dump(V);
	tb_trace(TB_WARN, "2str <%s>\n", tb_toStr(V,0));
	tb_Free(V);
	tb_Free(S);

	fm_Dump();
	
	//	*/
	return 0;
}

