// $Id: string_test.c,v 1.1 2004/05/12 22:05:14 plg Exp $
#ifdef __BUILD
#undef __BUILD
#endif


#include <sys/time.h>
#include <ctype.h>
#include <stdlib.h> 
#include <locale.h>

#include <stdio.h>
#include <string.h>

#define TB_MEM_DEBUG
#include "Toolbox.h"

#include "Objects.h"



String_t mytrim(String_t S);

String_t tb_strrepl(String_t S, char *search, char *replace) {
	if(tb_StrRepl(S,search, replace) == TB_OK) {
		tb_warn("S len = %d\n", tb_getSize(S));
		return S;
	} 
	tb_warn("failed\n");
	return S;
}

int main(int argc, char **argv) {
	String_t  C, D, S;
	char *s;

	int i, rc;
	Vector_t VA;
	Hash_t H;
	Num_t N;

	//	tb_errorlevel = TB_INFO;


	fprintf(stderr, "%d:%u:%d\n", UINT_MAX, UINT_MAX, TB_ERR);

	tb_profile("-starting\n");

	S = tb_String("12345");

	tb_profile("-testing tb_StrDel: [%s]\n", S2sz(S));
	fprintf(stderr, "1,1:\t[%s]\n", S2sz(tb_StrDel(S, 1, 1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "1,-1:\t[%s]\n", S2sz(tb_StrDel(S, 1, -1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "-3,1:\t[%s]\n", S2sz(tb_StrDel(S, -3, 1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "-3,-1:\t[%s]\n", S2sz(tb_StrDel(S, -3, -1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "0,1:\t[%s]\n", S2sz(tb_StrDel(S, 0, 1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "0,-1:\t[%s]\n", S2sz(tb_StrDel(S, 0, -1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "-1,1:\t[%s]\n", S2sz(tb_StrDel(S, -1, 1)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "-1,-1:\t[%s]\n", S2sz(tb_StrDel(S, -1, -1)));
	tb_Free(S);
	S = tb_String("12345");

	fprintf(stderr, "* 1,5:\t[%s]\n", S2sz(tb_StrDel(S, 1, 5)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "* -1,5:\t[%s]\n", S2sz(tb_StrDel(S, -1, 5)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "* 1,0:\t[%s]\n", S2sz(tb_StrDel(S, 1, 0)));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "* -1,0:\t[%s]\n", S2sz(tb_StrDel(S, -1, 0)));
	tb_Free(S);
	S = tb_String("12345");

	tb_profile("-testing tb_StrRepl: [%s]\n", S2sz(S));

	fprintf(stderr, "s/34/AB/:\t[%s]\n", 	S2sz(tb_strrepl(S, "34", "AB")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/34/CDE/:\t[%s]\n", S2sz(tb_strrepl(S, "34", "CDE")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/34/F/:\t[%s]\n", S2sz(tb_strrepl(S, "34", "F")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/34//:\t[%s]\n", S2sz(tb_strrepl(S, "34", "")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/12/AB/:\t[%s]\n", S2sz(tb_strrepl(S, "12", "AB")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/12/CDE/:\t[%s]\n", S2sz(tb_strrepl(S, "12", "CDE")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/12/F/:\t[%s]\n", S2sz(tb_strrepl(S, "12", "F")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/12//:\t[%s]\n", S2sz(tb_strrepl(S, "12", "")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/45/AB/:\t[%s]\n", S2sz(tb_strrepl(S, "45", "AB")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/45/CDE/:\t[%s]\n", S2sz(tb_strrepl(S, "45", "CDE")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/45/F/:\t[%s]\n", S2sz(tb_strrepl(S, "45", "F")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/45//:\t[%s]\n", S2sz(tb_strrepl(S, "45", "")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/12345//:\t[%s]\n", S2sz(tb_strrepl(S, "12345", "")));
	tb_Free(S);
	S = tb_String("12345");
	fprintf(stderr, "s/0123456//:\t[%s]\n", S2sz(tb_strrepl(S, "0123456", "AZAEZ")));
	tb_Free(S);
	S = tb_String("12345");


	fprintf(stderr, "%d\n", tb_getSize(S));

	tb_Free(S);

	fprintf(stderr, "delete twice object : error\n");
	tb_Free(S);

	
	tb_profile("check constructors/destructors\n");
	tb_profile("tb_String(NULL)");
	S = tb_String(NULL);
	tb_Dump(S);
	tb_Free(S);

	tb_profile("%s", "tb_String('%s : %d : %f', 'toto', 15, 4.3)\n");
	S = tb_String("%s : %d : %f", "toto", 15, 4.3);
	tb_Dump(S);
	tb_warn("%s", "tb_String('%S - %d', S, 25)\n");
	C = tb_String("%S - %d", S, 25);
	tb_Dump(C);
	tb_Free(S);
	tb_Free(C);

	tb_profile("%s", "tb_nString(5, '%s : %d : %f', 'toto', 15, 4.3)\n");
	S = tb_nString(6, "%s : %d : %f", "toto", 15, 4.3);
	tb_Dump(S);
	tb_profile("check Clear\n");
	tb_Dump(tb_Clear(S));
	tb_Free(S);
	tb_profile("check Clone\n");
	S = tb_String("%s : %d : %f", "toto", 15, 4.3);
	C = tb_Clone(S);


	tb_Dump(S);
	tb_Dump(C);
	tb_Free(C);

	tb_profile("check StrUcase\n");
	tb_Dump(tb_StrUcase(S));
	tb_profile("check StrLcase\n");
	tb_Dump(tb_StrLcase(S));


	C = tb_String("%s", "123456789");
	tb_profile("check StrSub('%s', 0, 1)\n", S2sz(C));
	tb_Dump(D = tb_StrSub(C, 0, 1));
	tb_Free(D);
	tb_profile("check StrSub('%s', -1, 1)\n", S2sz(C));
	tb_Dump(D = tb_StrSub(C, -1, 1));
	tb_Free(D);
	tb_profile("check StrSub('%s', 3, 2)\n", S2sz(C));
	tb_Dump(D = tb_StrSub(C, 3, 2));
	tb_Free(D);
	tb_profile("check StrSub('%s', -3, 2)\n", S2sz(C));
	tb_Dump(D = tb_StrSub(C, -3, 2));
	tb_Free(D);
	tb_profile("check StrSub('%s', -3, -1)\n", S2sz(C));
	tb_Dump(D = tb_StrSub(C, -3, -1));
	tb_Free(D);


	tb_profile("check bounds\n");

	D = tb_StrSub(C, 0, 0);
	if(D) tb_Free(D);
	D = tb_StrSub(C, -1, 0);
	if(D) tb_Free(D);
	D = tb_StrSub(C, 11, 1);
	if(D) tb_Free(D);
	D = tb_StrSub(C, 1, 11);
	if(D) tb_Free(D);

	tb_profile("check StrAdd\n");
	tb_Dump(C);

	tb_Free(S);

	S = tb_String(NULL);
	tb_profile("%s", " tb_StrAdd(S, 0, '%s', 'abcdefg')\n");
	tb_Dump(tb_StrAdd(S, 0, "%s", "abcdefg"));
	tb_profile("%s", " tb_StrAdd(S, 0, '%s/%d', '123', 4)\n");
	tb_Dump(tb_StrAdd(S, 0, "%s/%d", "123", 4));
	tb_profile("%s", " tb_StrAdd(S, -1, '%s/%2.2f', '123', 5.03)\n");
	tb_Dump(tb_StrAdd(S, -1, "%s/%2.2f", "123", 5.03));
	tb_profile("%s", "tb_StrAdd(S, 8, '%s', '.')\n");
	tb_Dump(tb_StrAdd(S, 8, "%s", "."));
	tb_profile("%s", "tb_StrnAdd(tb_Clear(S), 5, 0, '%s', '123456')\n");
	tb_Dump(tb_StrnAdd(tb_Clear(S), 5, 0, "%s", "123456"));
	tb_profile("%s", "tb_StrFill(S, 3, 1, '*')\n");
	tb_Dump(tb_StrFill(S, 3, 1, '*'));
	tb_profile("%s", "tb_StrRepl(C, '456', '-_-')\n");
	if(tb_StrRepl(C, "456", "-_-") == TB_OK) {
		tb_Dump(C);
	}
	tb_profile("%s", "tb_StrRepl(C, '-_-', '*')\n");
	if(tb_StrRepl(C, "-_-", "*") == TB_OK) {
		tb_Dump(C);
	}
	tb_profile("%s", "tb_StrRepl(C, '*78', '--AZE--')\n");
	if(tb_StrRepl(C, "*78", "--AZE--") == TB_OK) {
		tb_Dump(C);
	}

	tb_profile("%s", "tb_StrDel(C, -3, -1\n");
	tb_StrDel(C, -3, -1);
	tb_Dump(C);
	

	//	exit(0);


	tb_profile("check Num_t constructor\n");
	N = tb_Num(50);
	tb_Dump(N);
	fprintf(stderr, "[%d/%d]\n", N2int(N), tb_errno);
	tb_Clear(N);
	tb_Dump(N);
	fprintf(stderr, "[%d/%d]\n", N2int(N), tb_errno);
	tb_NumSet(N, 4555);
	tb_Dump(N);
	fprintf(stderr, "[%d/%d]\n", N2int(N), tb_errno);

	tb_Free(C);
	C = tb_String("ABCD");
	tb_crit("string: <%S>\n", C);
	D = tb_String("<%S:%D>", C, N);

	C = tb_StrAdd(tb_Clear(C), 0, "1234");
	fprintf(stderr, "[%s]\n", S2sz(D));
	fprintf(stderr, "[%s]/%d/%d\n", 
					tb_toStr(D), 
					tb_toInt(N), 
					tb_toInt(C));
	tb_Free(N);
	tb_Dump(D);
	tb_Free(D);
	tb_Clear(C);
	fprintf(stderr, "string: <%s>\n", S2sz(C));
	tb_Dump(C);     	
	tb_Free(C);


	C = NULL;
	tb_crit("tb_errno =%d (clone NULL : must fail)\n", tb_errno);
	tb_Clone(C);
	tb_crit("tb_errno =%d\n", tb_errno);

	VA = tb_Vector();

	tb_profile("check regexes\n");

	// setlocale(LC_CTYPE, "fr_FR"); 

	tb_Free(S);
	S = tb_String("ma chaîne de test a regexifier");
	tb_Dump(S);
	/*
	rc = tb_getRegex(VA, 
									 S, 
									 "^(\\w+).*(\\s\\S{2,2}\\s).*\\s(\\S*)$",
									 NULL,
									 PCRE_CASELESS);

	tb_profile("%d subtrings matched\n", rc);
	tb_Dump(VA);


	//	tb_Clear(VA);
	
	tb_Free(C);
	tb_Free(VA);
	*/
	//-
	H = tb_Hash();
	
	tb_profile("check 1000 tb_matchRegex\n");
	for(i = 0; i<1000; i++) {
		if(! tb_matchRegex(S, "aîn", 0)) exit(1);
	}
	tb_profile("done\n");

	
	if(tb_matchRegex(S, "t\\w+t", 0)) {
		tb_trace(TB_WARN, "found 't\\w+t' in string \n");
	} else {
		tb_trace(TB_WARN, "not found 't\\w+t' in string :(\n");
		exit(1);
	}


	tb_profile("sed/e./XX/ <%s>", S2sz(S));
	rc = tb_Sed("e.", "XX", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));

	tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
	tb_profile("1000 sed/(e)(.)/$2$1/ <%s> (no precomp)\n", S2sz(S));
	for(i = 0; i<1000; i++) {
		tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
		rc = tb_Sed("(e)(.)", "$2$1", S, PCRE_MULTI);
	}
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));

	tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
	tb_profile("1000 sed/(e)(.)/$2$1/ <%s> (with precomp)\n", S2sz(S));
	for(i = 0; i<1000; i++) {
		tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
		rc = tb_Sed("(e)(.)", "$2$1", S, PCRE_MULTI);
	}
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));


	tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");

	tb_profile("1 sed/(e)(.)/[$2$1]/ <%s>", S2sz(S));
	rc = tb_Sed("(e)(.)", "[$2$1]", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));


	tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
	tb_profile("2 sed/(e.)/--[$1]--/ <%s>", S2sz(S));
	rc = tb_Sed("(e.)", "--[$1]--", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));

	tb_StrAdd(tb_Clear(S), 0, "ma chaine de test a regexifier");
	tb_profile("3 sed/(e)(.)/-/ <%s>", S2sz(S));
	rc = tb_Sed("(e.)", "-", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));
		
	tb_StrAdd(tb_Clear(S), 0, "www.xanadu@cvf.fr");

	tb_profile("sed/([^\\.]*)(\\.)([^\\.]*)@/$1 point $2 @/ <%s>", S2sz(S));
	rc = tb_Sed("(\\.)", " point ", S, PCRE_MULTI);
	rc += tb_Sed("(@)", " at ", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));

	tb_profile("sed/\\s*at\\s*// <%s>", S2sz(S));
	rc = tb_Sed("\\s*at\\s*", "", S, PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));

	//dump_chunks(1);
	//dump_chunks(0);
	//dump_fm();

	tb_profile("sed <[^>]+>(.*)<[^>]+>  <%s>", S2sz(S));
	tb_Sed("<[^>]+>(.*)<[^>]+>", "$1", S,  
	       PCRE_UNGREEDY|PCRE_DOTALL|PCRE_MULTI);
	tb_profile("rc: %d <%s>\n", rc, S2sz(S));
	
	tb_Dump(S);


	tb_Free(VA);
	tb_StrAdd(tb_Clear(S), 0, "www.xanadu@cvf.fr");
	tb_Dump(VA = tb_StrSplit(S2sz(S), "([\\.@])", PCRE_MULTI));
	tb_Dump(C=tb_Join(VA, "*-*"));
	tb_Free(C);

	tb_freeArgv(tb_toArgv(VA));

	tb_StrAdd(tb_Clear(S), 0, "    <www.xanadu@cvf.fr>    ");
	tb_Dump(S);
	D = tb_Clone(S);
	tb_Free(D);
	//	tb_profile("<%s>\n", S2sz(tb_ltrim(S)));
	tb_errorlevel= TB_INFO;
	tb_profile("<%s>\n", S2sz(mytrim(S)));


	tb_StrAdd(tb_Clear(S), 0, "    <www.xanadu@cvf.fr>    ");
	tb_profile("<%s>\n", S2sz(tb_rtrim(S)));
	tb_StrAdd(tb_Clear(S), 0, "    <www.xanadu@cvf.fr>    ");
	tb_profile("<%s>\n", S2sz(tb_trim(S)));
	tb_profile("<%s>\n", S2sz(tb_trim(S)));
	tb_StrAdd(S, -1, "\n");
	tb_Dump(S);
	tb_profile("<%s>\n", S2sz(tb_chomp(S)));
	tb_chomp(S);
	tb_profile("<%s>\n", s = tb_str2hex(S2sz(S), tb_getSize(S)));
	tb_xfree(s);
  

	tb_profile("hexdump string\n");
	tb_hexdump(S2sz(S), tb_getSize(S));
	tb_profile("hexdump binary\n");
	tb_hexdump((char *)S, sizeof(struct tb_Object));


	tb_Free(VA);
	tb_Free(H);
	tb_Free(S);


	tb_profile("TB_ERR is now :%d\n", TB_ERR);
	
	return 0;
}
	

String_t mytrim(String_t S) {
	tb_Sed("^\\s*(\\S+)", "$1", S, PCRE_DOTALL);
	tb_Sed("(\\S+)\\s*$", "$1", S, PCRE_DOTALL);
	tb_Dump(S);
	return S;
}


