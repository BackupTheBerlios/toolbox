#ifdef __BUILD
#undef __BUILD
#endif


#include "Toolbox.h"


int main(int argc, char **argv) {
	String_t S = tb_String(NULL);
	Date_t A,B,C,D = tb_Date("20040513T23:06:07");

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

	return 0;
}
