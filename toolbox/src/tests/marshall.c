// $Id: marshall.c,v 1.4 2004/05/24 16:37:53 plg Exp $

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <assert.h>
#include <stdio.h>
#include "Toolbox.h"


int string_test(void);
int raw_test(void);
int num_test(void);
int vector_test(void);
int hash_test(void);
int mixed_test(void);


int main(int argc, char **argv) {

	tb_profile("string_test");
	string_test();

	tb_profile("raw_test");
	raw_test();

	tb_profile("num_test");
	num_test();

	tb_profile("vector_test");
	vector_test();

	tb_profile("hash_test");
	hash_test();

	tb_profile("mixed_test");
	mixed_test();
 
	return 0;
}


int string_test() {
	String_t U, M,  S = tb_String("hello string");

	tb_Dump(S);
	
	M = tb_Marshall( S );

	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

 
	U = tb_unMarshall( M );

	tb_Dump(U);

	return TB_OK;
}


int raw_test() {
	Raw_t U, M,  S = tb_Raw(10, "hello string");

	tb_Dump(S);

	M = tb_Marshall( S );
	
	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

	U = tb_unMarshall( M );

	tb_Dump(U);

	return TB_OK;
}

int num_test() {
	Num_t U, M,  S = tb_Num(10);

	tb_Dump(S);

	M = tb_Marshall( S );
	
	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

	U = tb_unMarshall( M );

	tb_Dump(U);

	return TB_OK;
}

int vector_test() {
	Vector_t U,  V = tb_Vector();
	String_t M;

	tb_Push(V, tb_String("1"));
	tb_Push(V, tb_String("2"));
	tb_Push(V, tb_String("3"));

	tb_Dump(V);

	M = tb_Marshall( V );
	
	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

	U = tb_unMarshall( M );

	tb_Dump(U);

	return TB_OK;
}

int hash_test() {
	Hash_t U,  H = tb_Hash();
	String_t M;

	tb_Insert(H, tb_String("1"), "un");
	tb_Insert(H, tb_String("2"), "deux");
	tb_Insert(H, tb_String("3"), "trois");

	tb_Dump(H);

	M = tb_Marshall( H );
	
	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

	U = tb_unMarshall( M );

	tb_Dump(U);

	return TB_OK;
}


int mixed_test() {
	Hash_t U,T,  H = tb_Hash();
	String_t M;
	Vector_t V = tb_Vector();

	T = tb_Hash();

	tb_Insert(H, tb_String("1"), "un");
	tb_Insert(H, tb_Raw(5, "deux"), "deux");
	tb_Insert(H, tb_String("3"), "trois");

	tb_Push(V, tb_String("4"));

	tb_Push(V, T);
	tb_Insert(T, tb_String("5"), "cinq");
	tb_Insert(T, tb_Num(6), "six");
	tb_Insert(V, tb_String("7"));

	tb_Insert(H, V, "array");

	tb_Dump(H);

	M = tb_Marshall( H );
	
	fprintf(stderr, "--\n%s\n--\n", S2sz(M));

	U = tb_unMarshall( M );

	tb_Dump(U);

	tb_Free(M);

	tb_profile("stringification\n");

	M = tb_Stringify(H);

	fprintf(stderr, "Stringified:\n%s\n", tb_toStr(M));

	tb_Free(M);



	return TB_OK;
}




