#ifdef __BUILD
#undef __BUILD
#endif

#include <stdio.h>
#include "Toolbox.h"


int main(int argc, char **argv) {
	String_t S = tb_String(NULL);
	Date_t A,B,D = tb_Date("20040513T23:06:07");

	tb_Dump(D);
	tb_warn("c-cast : d:%d s:%s\n", tb_toInt(D), tb_toStr(D));
	A = tb_Clone(D);
	tb_Dump(A);
	B = tb_Date("1989-03-08T23:06:07");
	tb_Dump(B);

	if(tb_DateCmp(A,B) >0) {
		tb_warn("%s > %s\n", tb_toStr(A), tb_toStr(B));
	} else {
		tb_warn("%s < %s\n", tb_toStr(A), tb_toStr(B));
	}

	S = tb_Marshall(D);
	fprintf(stderr, "Marshalled:\n%s\n", tb_toStr(S));
	tb_Free(D);
	tb_Dump(D = tb_unMarshall(S));
	tb_Free(A);
	tb_Free(B);
	tb_Free(D);

	return 0;
}
