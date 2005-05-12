// $Id: xml_test.c,v 1.2 2005/05/12 21:52:41 plg Exp $
#undef __BUILD
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "Toolbox.h"

String_t slurpFile(char *file);

int main(int argc, char **argv) {

	XmlElt_t Root;
	Hash_t Attr;
	XmlDoc_t Doc;
	/*
	char *tstfiles[] = { "soap.xml", 
											 "tst2.xml", 
											 "tst3.xml", 
											 NULL };
	*/
	//	char *tstfiles[] = { "soap.xml", NULL };
	char *tstfiles[] = { "test.xml", NULL };
//	char *tstfiles[] = { "utf8.xml", NULL };
	char **file = tstfiles;


	//	char tst[]="%3C%3Fxml+version%3D%221.0%22+encoding%3D%22UTF-8%22%3F%3E+%0A%3CPUSH_CLIENT+ENCRYPTED%3D%22No%22+PRIORITY%3D%22NORMAL%22%3E%0A++%3CCONTROL%3E%0A++++%3COFFER+IDOFFER%3D%22RQSCVFTOURDEFRANCE%22%2F%3E%0A++%3C%2FCONTROL%3E%0A++%3CCONTENT+FORMAT%3D%22A%22+TIMESTAMP%3D%22%22%3E%0A++++%3CFROM%3E%3C%2FFROM%3E%0A++++%3CUSER+TYPE%3D%22MSISDN%22+VALUE%3D%22663130190%22%2F%3E%0A++++%3CSUBJECT%3E%3C%2FSUBJECT%3E%0A++++%3CTEXT+LENGTH%3D%2211%22%3EON+A+ETE%C3%83%C2%A8%C3%83%C2%AC%C3%83%3F%3C%2FTEXT%3E%0A++%3C%2FCONTENT%3E%0A%3C%2FPUSH_CLIENT%3E";

	char tst[]="%3C%3Fxml+version%3D%221.0%22+encoding%3D%22UTF-8%22%3F%3E+%0A%3CPUSH_CLIENT+ENCRYPTED%3D%22No%22+PRIORITY%3D%22NORMAL%22%3E%0A++%3CCONTROL%3E%0A++++%3COFFER+IDOFFER%3D%22RQSCVFTOURDEFRANCE%22%2F%3E%0A++%3C%2FCONTROL%3E%0A++%3CCONTENT+FORMAT%3D%22A%22+TIMESTAMP%3D%22%22%3E%0A++++%3CFROM%3E%3C%2FFROM%3E%0A++++%3CUSER+TYPE%3D%22MSISDN%22+VALUE%3D%22661577662%22%2F%3E%0A++++%3CSUBJECT%3E%3C%2FSUBJECT%3E%0A++++%3CTEXT+LENGTH%3D%2249%22%3EEST+CE+QUE+L%27%C3%83%3FQUIPE+US+POSTAL+PEUT+ENCORE+GAGNER%3F%3C%2FTEXT%3E%0A++%3C%2FCONTENT%3E%0A%3C%2FPUSH_CLIENT%3E";

	tb_errorlevel = TB_NOTICE;

	

	do {
		String_t S = slurpFile( *file++ );
//		String_t S = tb_String("%s", tst);

/* 		tb_UrlDecode(S); */
/* 		tb_Dump(S); */
/* 		//		tb_UTF8_to_Latin1(S); */
/* 		tb_Dump(S); */
/* 		//tb_UTF8_to_Latin1(S); */
/* 		tb_Dump(S); */

		//		tb_Dump(tb_UrlEncode(S));
		//tb_Dump(tb_UrlDecode(S));


		tb_profile("start parsing\n");
		Doc = tb_XmlDoc(S2sz(S));
		if(! Doc) exit(1);

		tb_profile("doc parsed\n");
		tb_Free(S);
		Root = XDOC_getRoot(Doc);

		String_t Reverse = XDOC_to_xml(Doc);
		fprintf(stderr, "%s", tb_toStr(Reverse));


		exit(0);

		/*
		Root = XDOC_getRoot(Doc);
		tb_Dump( Root );
		*/
		tb_profile("+Dom_to_xml\n");
		S = XDOC_to_xml(Doc);
		tb_Dump(S);

		tb_profile("-Dom_to_xml\n");
		//		tb_Dump( S );
		tb_Free(S);
		tb_profile("+free doc\n");
		tb_Free(Doc);
		tb_profile("-free doc\n");
		exit(0);
	} while( *file );

	exit(0);

	fm_Dump();

	Doc = tb_XmlDoc(NULL);
	Attr = tb_Hash();
	tb_Replace(Attr, tb_String("oui"), "att1");
	tb_Replace(Attr, tb_String("non"), "att2");
	tb_Replace(Attr, tb_String("non"), "att3");
	tb_Dump(Attr);
	Root = tb_XmlNodeElt(NULL, "rootelt", Attr);
	XDOC_setRoot(Doc, Root);
	
	fprintf(stderr, "%s\n", S2sz(XDOC_to_xml(Doc)));

	//	fm_dumpChunks();

	return 0;
}

	


String_t slurpFile(char *file) {
	int rc=1, fd;
	String_t S = NULL;
	char buff[4096];
	tb_profile(" ------------------------ test file : %s ------\n", file);

	assert(file != NULL);
	assert((fd = open(file, O_RDONLY)) != -1);
	S = tb_String(NULL);
	while( rc > 0) {
		rc = read(fd, buff, 4095);
		//		tb_warn("read %d bytes\n", rc);
		if(rc >0 ) {
			buff[rc] = 0;
			//			fprintf(stderr, "++\n%s\n++\n", buff);
			tb_StrAdd(S, -1, "%s", buff);
			//			tb_Dump(S);
		}
	}
	assert(rc != -1);
	//	fprintf(stderr, "\n--\n%s\n--\n", S2sz(S));
	tb_profile("file loaded\n");
	return S;
}






