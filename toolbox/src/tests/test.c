//#define TB_MEM_DEBUG

#define GC_DEBUG
#include <string.h>
// overloaded fncs
#ifdef WITH_GC
#define LINUX_THREADS
#define _REENTRANT
#include <gc.h>
#include <pthread.h>
#define malloc(n) GC_malloc(n)
#define calloc(m,n) GC_malloc((m)*(n))
#define realloc(m,n) GC_realloc((m),(n))
#undef strdup
#define strdup(a)  memcpy(GC_malloc(strlen(a)+1), a, strlen(a))
#define free(a) GC_free(a)
#endif

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include "Toolbox.h"
#include <syslog.h>

#include "Dict.h"

/*
#define TEST_VARRAY
#define TEST_HASH
#define MIXED_TYPES
#define TEST_TOKENIZE
#define TEST_MSQ
#define TEST_SLOCK
#define TEST_SOCKET
#define TEST_REGEX
#define TEST_USER_OBJECT
#define TEST_MEMORY
define TEST_STRING
#define TEST_DICT
//#define TEST_CONFIG
#define TEST_GC
*/

//#define TEST_SORT
//#define TEST_GC
//#define TEST_HASH
#define TEST_STRING

void bigalloc(void);
void doIt(void *VC, void *data);
int search(void *vc, void *data);
int callback(void *arg);

int main(void) {


  int i, rc, ndx;

  Msq_t *MQ    = new_Msq(".", 'a', 254);
	SLock_t *Sl  = new_SLock(".", 'b'); 

  ps_info_t I;
	
	GC_enable_incremental();

	// tb_Obj_t O;
	//	mtrace();
	//	tb_memDebug();

	//	tb_errorlevel = TB_DEBUG;

  setvbuf(stdout, NULL, _IONBF , 0); 
  setvbuf(stderr, NULL, _IONBF , 0); 

	//	openlog("test_Toolbox", LOG_PERROR|LOG_PID, LOG_USER);

#ifdef TEST_CONFIG
	if(1) {
		Hash_t H = tb_New(TB_HASH);
		tb_readConfig(H, "./config.tst", "([\\s:=]+)", "([\\s,]+)", 0);
		tb_Dump(H);
		tb_Free(H);
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
	}

#endif

#ifdef TEST_STRING
	if(1) {
		String_t S;
		tb_Obj_t O;
		S = tb_New(TB_STRING, "ab cd ef gh");

		fprintf(stderr, "base string : <%s>\n", S2sz(S));		
		fprintf(stderr, "add ' ij', -1 : <%s>\n", S2sz(tb_StrAdd(S, " ij", -1)));		
		fprintf(stderr, "add '01 ', 0  : <%s>\n", S2sz(tb_StrAdd(S, "01 ", 0)));		

		fprintf(stderr, "cut 1, 1     : <%s>\n", S2sz(tb_StrCut(S, 1, 1)));		
		fprintf(stderr, "cut -1, 1    : <%s>\n", S2sz(tb_StrCut(S, -1, 1)));		
		fprintf(stderr, "cut 3, 2     : <%s>\n", S2sz(tb_StrCut(S, 3, 2)));		
		fprintf(stderr, "cut -3, 2    : <%s>\n", S2sz(tb_StrCut(S, -3, 2)));		
		fprintf(stderr, "cut 3, -1    : <%s>\n", S2sz(tb_StrCut(S, 3, -1)));		
		fprintf(stderr, "add 'b c d e f g h ij', -1 : <%s>\n", 
						S2sz(tb_StrAdd(S, "b c d e f g h ij", -1)));		
		fprintf(stderr, "cut -3, -1   : <%s>\n", S2sz(tb_StrCut(S, -3, -1)));		
		fprintf(stderr, "add 'xx', 5  : <%s>\n", S2sz(tb_StrAdd(S, "xx", 5)));		
		fprintf(stderr, "add 'xx', -4 : <%s>\n", S2sz(tb_StrAdd(S, "xx", -4)));		
		fprintf(stderr, "add '', 0    : <%s>\n", S2sz(tb_StrAdd(S, "", 0)));		
		fprintf(stderr, "add '', -1   : <%s>\n", S2sz(tb_StrAdd(S, "", -1)));		
		fprintf(stderr, "add '', 3    : <%s>\n", S2sz(tb_StrAdd(S, "", 3)));		

		fprintf(stderr, "extract -1, 1  : <%s> -> <%s>\n", S2sz(S), S2sz(O = tb_subStr(S, -1,1)));	
		tb_Free(O);
		fprintf(stderr, "extract -4, 5  : <%s> -> <%s>\n", S2sz(S), S2sz(O = tb_subStr(S, -4,5)));	
		tb_Free(O);
		fprintf(stderr, "extract 4, 2   : <%s> -> <%s>\n", S2sz(S), S2sz(O = tb_subStr(S, 4,2)));	
		tb_Dump(O);
		tb_Free(O);
		fprintf(stderr, "extract 4, -1  : <%s> -> <%s>\n", S2sz(S), S2sz(O = tb_subStr(S, 4,-1)));	
		tb_Free(O);
		fprintf(stderr, "extract -4, -1 : <%s> -> <%s>\n", S2sz(S), S2sz(O = tb_subStr(S, -4,-1)));	
		tb_Free(O);
		while(tb_StrRepl(S, "xx", "-"));
		fprintf(stderr, "replace 'xx' '-' : <%s> \n", S2sz(S));	
		fprintf(stderr, "enlarge 2, 5, 'x' : <%s> \n", S2sz(tb_StrEnlarge(S, 5, 0, 'x')));	
		tb_Free(S);
		//		dumpSelfCheck(selfCheck(&I));
		fprintf(stderr, "test STRINGS passed\n");
		//		tb_memDump();
	}
#endif

	/*
#ifdef TEST_MEMORY
	if(1) {
		void *v;

		//		tb_errorlevel = TB_DEBUG;
		v = malloc(100);
		fprintf(stderr, "100 bytes mallocated on %p\n", v);
		sprintf(v ,"hello");
		fprintf(stderr, "free %p\n", v);
		free(v);
		//		tb_checkAddr(v, __FILE__, __LINE__);
		v = realloc(v, 120);
		v = calloc(1, 100);
		v = calloc(1, 100);
		v = calloc(1, 100);
		tb_checkAddr(v -1, __FILE__, __LINE__);
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
		//		tb_memDestroy(__FILE__, __LINE__);
		//		tb_memDump();
	}


#endif
	*/
#ifdef TEST_REGEX
	if(1) {
		String_t S;
		int i, rc;
		Varray_t VA;
		Hash_t H;
		struct timeval t1, t2;

		fprintf(stderr, "Heap size: %d\n", GC_get_heap_size());

		VA = tb_New(TB_VARRAY);
		tb_errorlevel = TB_WARN;

		tb_Clear(VA);
		S = tb_New(TB_STRING, "ma chaine de test a regexifier");

		tb_Dump(S);

		rc = tb_getRegex(VA, 
										 S, 
										 "^(\\w+).*(\\s\\S{2,2}\\s).*\\s(\\S*)$",
										 NULL,
										 PCRE_CASELESS);

		tb_trace(TB_WARN, "%d subtrings matched\n", rc);
		tb_Dump(VA);
		tb_Clear(VA);
		tb_Dump(VA);

		H = tb_New(TB_HASH);

		i = 0;
		gettimeofday(&t1, NULL);
		for(i = 0; i<1000; i++) {
			if(tb_matchRegex(S, "\\d", NULL, 0)) {
				exit(1);
			} else {
				i++;
			}
		}
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test1: %d.%d sec, matched %d\n", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, i);
		
		i = 0;
		gettimeofday(&t1, NULL);
		for(i = 0; i<1000; i++) {
			if(tb_matchRegex(S, "\\d", H, 0)) {
				exit(1);
			} else {
				i++;
			}
		}
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test2: %d.%d sec matched %d\n", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, i);


		if(tb_matchRegex(S, "t\\w+t", H, 0)) {
			tb_trace(TB_WARN, "found 't\\w+t' in string \n");
		} else {
			tb_trace(TB_WARN, "not found 't\\w+t' in string :(\n");
			exit(1);
		}


		tb_trace(TB_WARN, "sed/e./XX/ <%s>", S2sz(S));
		rc = tb_Sed("e.", "XX", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s> (%d)\n", S2sz(S), rc);

		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);

		tb_trace(TB_WARN, "sed/(e)(.)/$2$1/ <%s>\n", S2sz(S));
		gettimeofday(&t1, NULL);
		rc = tb_Sed("(e)(.)", "$2$1", S, NULL, NULL, 0,1);
		tb_trace(TB_WARN, "<%s> (%d)\n", S2sz(S), rc);
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test3: %d.%d sec [%d] (no precomp)", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, rc);


		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);
		tb_trace(TB_WARN, "sed/(e)(.)/$2$1/ <%s>\n", S2sz(S));

		gettimeofday(&t1, NULL);
		rc = tb_Sed("(e)(.)", "$2$1", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S), rc);
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test4: %d.%d sec [%d] (precomp)", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, rc);

		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);
		tb_trace(TB_WARN, "sed/(e)(.)/$2$1/ <%s>\n", S2sz(S));
		gettimeofday(&t1, NULL);
		rc = tb_Sed("(e)(.)", "$2$1", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S), rc);
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test5: %d.%d sec [%d] (precomp again)", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, rc);

		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);
		tb_trace(TB_WARN, "sed/(e)(.)/$2$1/ <%s>\n", S2sz(S));
		gettimeofday(&t1, NULL);
		rc = tb_Sed("(e)(.)", "$2$1", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S), rc);
		gettimeofday(&t2, NULL);
		tb_trace(TB_CRIT, "test6: %d.%d sec [%d] (precomp again)", 
						 t2.tv_sec - t1.tv_sec, t2.tv_usec - t1.tv_usec, rc);

	// test normal

		tb_errorlevel = TB_NOTICE;
		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);

		tb_trace(TB_WARN, "sed/(e)(.)/[$2$1]/ <%s>", S2sz(S));
		rc = tb_Sed("(e)(.)", "[$2$1]", S, H, NULL, 0, 1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S));

	// test from = 0

		tb_errorlevel = TB_NOTICE;
		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);

		tb_trace(TB_WARN, "sed/(ma)/-/ <%s>", S2sz(S));
		rc = tb_Sed("(e.)", "--[$1]--", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S));

		tb_StrAdd(tb_Clear(S), "ma chaine de test a regexifier", 0);
		tb_trace(TB_WARN, "sed/(e)(.)/-/ <%s>", S2sz(S));
		rc = tb_Sed("(e.)", "-", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S));
		
		//		tb_StrAdd(tb_Clear(S), "www.xanadu@cvf.fr", 0);
		tb_StrAdd(tb_Clear(S), "123456789123456789", 0);

		tb_trace(TB_WARN, "sed/([^\\.]*)(\\.)([^\\.]*)@/$1 point $2 @/ <%s>", S2sz(S));
		tb_Sed("(\\.)", " point ", S, H, NULL, 0,1);
		tb_Sed("(@)", " at ", S, H, NULL, 0,1);
		tb_trace(TB_WARN, "<%s>\n", S2sz(S) );

		
		tb_Clear(VA);
tb_StrAdd(tb_Clear(S), "Nathalie PRÉVÔT - CVF +33 5 56 01 98 12 mailto:natha\nlie `DOT` prevot @ cvf `DOT` fr.", 0);
//		tb_StrAdd(tb_Clear(S), "123 `4` 56 `789` ABCDEF `HSJ` `p` ", 0);
		tb_getRegex(VA, S, "(`[^`]+`)", NULL, PCRE_MATCHMULTI);
		//tb_getRegex(VA, S, "`([^`]+)`", NULL, 0);
		tb_Dump(VA);

		//  tb_errorlevel = TB_DEBUG;
		tb_Free(VA);
		tb_Free(H);
		tb_Free(S);
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
		fprintf(stderr, "Heap size: %d\n", GC_get_heap_size());
		GC_gcollect();
		fprintf(stderr, "Heap size: %d\n", GC_get_heap_size());
	}
#endif

#ifdef TEST_MSQ
  dumpSelfCheck(selfCheck(&I));
  system("ipcs -q");
  sprintf(MQ->Msg->mtext, "mon message");

  Msq_send(MQ, 1, IPC_NOWAIT);
  system("ipcs -q");
  if(Msq_peek(MQ, 1)) {
    tb_trace(TB_WARN, "Message en attente\n");
    if(Msq_read(MQ, 1, IPC_NOWAIT)) {
      system("ipcs -q");
      printf("Msg<%s> type<%ld>\n", (char *)MQ->Msg->mtext, MQ->Msg->type);
    } else {
      tb_trace(TB_WARN, "failed to retrieve msg\n");
      exit(1);
    }
  } else {
    tb_trace(TB_WARN, "Message perdu :(\n");
    exit(1);
  }
  Msq_chmod(MQ, "666");
  system("ipcs -q");
  Msq_chmod(MQ, "664");
  system("ipcs -q");
  Msq_remove(MQ);
  system("ipcs -q");
#endif

#ifdef TEST_SLOCK
  dumpSelfCheck(selfCheck(&I));
  system("ipcs -s");
  if(SLock_lock(Sl, IPC_NOWAIT)) {
    tb_trace(TB_WARN, "sem locked\n");
    system("ipcs -s");
  } else {
    tb_trace(TB_WARN, "can't get sem locked\n");
  }
  if(! SLock_lock(Sl, IPC_NOWAIT)) {
    tb_trace(TB_WARN, "sem yet locked\n");
  } else {
    tb_trace(TB_WARN, "sem re-locked (err)\n");
  }
  if(SLock_unlock(Sl, IPC_NOWAIT)) {
    tb_trace(TB_WARN, "sem unlocked\n");
    system("ipcs -s");
  }
  SLock_chmod(Sl, "664");
  system("ipcs -s");
  SLock_remove(Sl);
  system("ipcs -s");
#endif

#ifdef TEST_SOCKET
	//	tb_errorlevel = TB_DEBUG;
#define DEBUG_SOCK
	if(1) {
		tb_Obj_t So = tb_New(TB_SOCKET);

		dumpSelfCheck(selfCheck(&I));
		tb_Dump(So);

		tb_trace(TB_WARN, "init socket rc = %d\n", 
						 //						 tb_initSocket(So, LAZARE, 20100));
						 tb_initSocket(So, "pop.free.fr", 143));
		tb_Dump(So);
		/*
		if(tb_getSockStatus(So) == TB_CONNECTED) {
			tb_writeSock(So, "TEST Toolbox\n");
		} else {
			tb_trace(TB_WARN, "Not connected !!\n");
		}

		Lazarize("TEST:%d\n", 2);
		*/
		dumpSelfCheck(selfCheck(&I));
		tb_Free(So);
		//		tb_memDump();
	}
	if(0) {
		tb_Obj_t Server = tb_New(TB_SOCKET);
		tb_Obj_t Client = tb_New(TB_SOCKET);
		pthread_t pt;
		char buff[MAX_BUFFER+1];

		//		tb_errorlevel = TB_DEBUG;
		tb_trace(TB_WARN, "init :%d\n", 
						 tb_initSockServer(Server, 55555, callback, NULL));
		tb_Dump(Server);
		//		tb_Accept(Server);

		
		pthread_create(&pt, NULL, (void *)tb_Accept, Server);
		pthread_detach(pt);

		tb_initSocket(Client, "localhost", 55555);
		tb_Dump(Client);
		tb_writeSock(Client, "Hello");
		tb_readSock(Client, buff, MAX_BUFFER);
		tb_trace(TB_WARN, "Client: recv<%s>\n", buff);
		tb_Free(Client);

		//		tb_Free(Server);
		dumpSelfCheck(selfCheck(&I));		
		//		sleep(10);
		//		tb_memDump();
	}
#endif
	


#ifdef TEST_VARRAY
	if(1) {
		int ndx, retval = 0;
		char l1[] = "test 1";
		char l2[] = "test - 2";
		char l3[] = "test -- 3";
		char buffer[255];
		Varray_t VA = tb_New(TB_VARRAY);
		tb_Obj_t O;

		tb_trace(TB_NOTICE, "push 3\n");
		tb_Add(VA, tb_New(TB_STRING, "[%s]:%d", l1, 1));
		tb_Add(VA, tb_New(TB_STRING, NULL));
		tb_Add(VA, tb_New(TB_STRING, l3));
		tb_Dump(VA);

		tb_trace(TB_NOTICE, "Peek '1'");
		O = tb_Peek(VA, 1);
		fprintf(stderr, "Peek 1 <%s> Peeksz 1 : %d\n", S2sz(O), tb_Peeksz(VA, 1));

		fprintf(stderr, "pop 4\n");
	
		fprintf(stderr, "1<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "2<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "3<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "4<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		tb_Dump(VA);
		fprintf(stderr, "push 3\n");
		tb_Add(VA, tb_New(TB_STRING, "[%s]:%d", l1, 2));
		tb_Add(VA, tb_New(TB_STRING, l2));
		tb_Add(VA, tb_New(TB_STRING, l3));

		tb_Dump(VA);

		tb_trace(TB_NOTICE, "Get 1 <%s>\n", S2sz(O = tb_Get(VA, 1)));
		tb_Dump(VA);
		tb_trace(TB_NOTICE, "re-enter string\n");
		tb_Add(VA, O);
		tb_Dump(VA);
		tb_trace(TB_NOTICE, "Remove 1\n");
		tb_Remove(VA, 1);
		tb_Dump(VA);


		fprintf(stderr, "shift 4\n");
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Shift(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Shift(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Shift(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Shift(VA)));
		tb_Free(O);
		tb_Dump(VA);

		fprintf(stderr, "unshift 3\n");
		tb_Unshift(VA, tb_New(TB_STRING, l1));
		tb_Unshift(VA, tb_New(TB_STRING, l2));
		tb_Unshift(VA, tb_New(TB_STRING, l3));
		tb_Dump(VA);
		fprintf(stderr, "pop 4\n");
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		fprintf(stderr, "<%s>\n", S2sz(O = tb_Pop(VA)));
		tb_Free(O);
		tb_Dump(VA);
		fprintf(stderr, "add 3, unshift 3\n");
		tb_Add(VA, tb_New(TB_STRING, "[%s]:%d", l1, 1));
		tb_Add(VA, tb_New(TB_STRING, l2));
		tb_Add(VA, tb_New(TB_STRING, l3));
		tb_Unshift(VA, tb_New(TB_STRING, l1));
		tb_Unshift(VA, tb_New(TB_STRING, l2));
		tb_Unshift(VA, tb_New(TB_STRING, l3));
		tb_Dump(VA);
		fprintf(stderr, "foreach test: reverse print each cell interlacing with ':'\n");
		sprintf(buffer, ":");
		tb_Foreach(VA, (void *)doIt, buffer);
		fprintf(stderr, "search string '2' in VArray :\n");
		ndx = tb_Search(VA, (void *)search, "2", &retval);
		printf("found <%s> in ndx %d\n", S2sz(tb_Peek(VA, ndx)), ndx);
		tb_Dump(VA);

		tb_Free(VA);

		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();

	}
#endif

#ifdef TEST_HASH
	if(1) {
		int i;
		char *key[1000], *value[1000];
		Varray_t VA;
		Hash_t H = tb_Shareable(tb_New(TB_HASH));
		//  dumpSelfCheck(selfCheck(&I));
		//		tb_errorlevel = TB_DEBUG;


		for(i = 0; i < 1000; i++) {
			key[i]   = (char *)malloc(sizeof(char) * 5);
			value[i] = (char *)malloc(sizeof(char) * 30);
			sprintf(key[i], "%04d", i);
			sprintf(value[i], "test value %04d", i);
			tb_Add(H, tb_New(TB_STRING, value[i]), key[i]);
			free(key[i]);
			free(value[i]);
			fprintf(stderr, "added %d\n", i);
		}

		tb_Dump(H);
		fprintf(stderr, "hash_peek(0003) : <%s>\n", tb_toStr(H, "0003"));
		tb_Add(H, tb_New(TB_STRING, "new value"), "0003");
		fprintf(stderr, "insert a new value for same key\n");
		fprintf(stderr, "hash_lookup(0003) : <%s>\n", tb_toStr(H, "0003"));
		fprintf(stderr, "hash_lookup('inexistant') : <%s>\n", tb_toStr(H, "inexistant"));
		fprintf(stderr, "is key '0003' exists ? rc = %d\n", tb_Exists(H, "0003"));
		fprintf(stderr, "remove key 0003\n");
		tb_Remove(H, "0003");
		fprintf(stderr, "is key '0003' exists ? rc = %d\n", tb_Exists(H, "0003"));
		VA = hash_keys(H);
		tb_Dump(VA);

		tb_trace(TB_WARN, "key[3] = <%s>\n", tb_toStr(VA, 3));

		fprintf(stderr, "%d keys loaded\n", tb_getSize(VA));
		tb_Free(VA);
		fprintf(stderr, "VA freed\n");

		for(i = 0; i < 1000; i += 3) {
			char key[5];
			sprintf(key, "%04d", i);
			fprintf(stderr, "try to remove key <%s>\n", key);
			tb_Remove(H, key);
			//			tb_Dump(tb_Get(H, key));
		}


		tb_Clear(H);
		fprintf(stderr, "H cleared\n");
		tb_Dump(H);
		//		tb_memDump();
		tb_Free(H);
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
	}
#endif

#ifdef TEST_TOKENIZE
	if(1) {
		char cutme[]=":test:de:chaine..a:.decouper:en:tokens:";
		char cutmerx[]="test : de := chaine  .. a:.decouper:en:tokens:";
		char cutmeQT[]="test : 'de := chaine  ..\" 'a:.\" 'decouper:en':tok\"ens:";
		Varray_t VA;
		String_t O;
		dumpSelfCheck(selfCheck(&I));
		//	tb_errorlevel = TB_DEBUG;
		VA = tb_New(TB_VARRAY);
		fprintf(stderr, "tokenize('%s', ':.')\n", cutme);
		tb_tokenize(VA, cutme, ":.", 1);
		tb_Dump(VA);
		fflush(stderr);

		fprintf(stderr, "joined with '/' :\n<%s>\n", S2sz(O = tb_Join(VA, "/")));
		tb_Clear(VA);
		tb_Free(O);

		fprintf(stderr, "tokenize('%s', '.:') (w/ blanks)\n", cutme);
		tb_tokenize(VA, cutme, ".:", 0);
		tb_Dump(VA);
		fprintf(stderr, "add tokenize('1:+33', ':') (w/o blanks)\n");
		tokenize(VA, "1:+33", ":");
		tb_Dump(VA);
		fprintf(stderr, "joined with ':' :\n<%s>\n", S2sz(O = tb_Join(VA, ":")));
		tb_Clear(VA);
		tb_Free(O);


		fprintf(stderr, "Tokenize on regex ([\\s:=\\.]+) <%s> (once)\n", cutmerx);
		tb_tokenizeRx(VA, cutmerx, "([\\s:=\\.]+)", 0, 0);
		tb_Dump(VA);
		tb_Clear(VA);
		fprintf(stderr, "Tokenize on regex ([\\s:=\\.]+) <%s> (multi)\n", cutmerx);
		tb_tokenizeRx(VA, cutmerx, "([\\s:=\\.]+)", 0, 1);
		tb_Dump(VA);

		tb_Clear(VA);
		fprintf(stderr, "Tokenize on regex ([\\s:=\\.]+) <%s> (multi)\n", cutmerx);
		tb_tokenizeRx(VA, cutmerx, "([\\s:=\\.]+)", 0, 1);
		tb_Dump(VA);

		tb_Free(VA);
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
	}
#endif

#ifdef MIXED_TYPES
	dumpSelfCheck(selfCheck(&I));

	//	tb_errorlevel = TB_DEBUG;

  fprintf(stderr, "Mixed types : Hash of VArrays :\n");
  if( 1 ) {
    int i;
    char k[10][10], **ARGV;
    Varray_t V[10], X;
    Hash_t H = tb_New(TB_HASH);
		//    tb_Obj_t obj;
		char cutme[]=":test:de:chaine..a:.decouper:en:tokens:";
		void *v;

    for(i = 0; i<10; i++) {
      sprintf(k[i], "%02d", i);
			tb_Add(H, V[i] = tb_New(TB_VARRAY), k[i]);
      tokenize(V[i], cutme, ":.");
      tb_Unshift(V[i], tb_New(TB_STRING, k[i]));
    }
    tb_Dump(H);
    fprintf(stderr, "extract H{04}\n");
		fflush(stderr);
    X = tb_Get(H, "04");
    tb_Dump(X);
    fprintf(stderr, "show resulting hash\n");
    tb_Dump(H);
    fprintf(stderr, "now free Hash and array\n");
    tb_Free(H);
    tb_Free(X);
    
		V[0] = tb_New(TB_VARRAY);
		fprintf(stderr, "Return a char * array, null terminated :\n");
		tokenize(V[0], cutme, ":.");
		
		assert((v = ARGV = tb_toArgv(V[0])) != NULL);
		
		//		tb_memDump();

		while(*ARGV) {
			printf("<%s>\n", *ARGV); 
			free(*ARGV++);
		}
		free(v);


		tb_Free(V[0]);
    
		dumpSelfCheck(selfCheck(&I));
		//		tb_memDump();
  }
#endif

#ifdef TEST_USER_OBJECT

  if(1) {
		tb_Obj_t O;
    Obj_def_t *mydef = calloc(1, sizeof(Obj_def_t));
    uint newType;

		//    tb_errorlevel = TB_DEBUG;

    mydef->childOf = TB_STRING;

    newType = tb_registerObject(mydef);
    tb_trace(TB_WARN, "new type is :%d\n", newType);
    O = tb_New(newType, "test nouveau type");
    tb_Dump(O);
		tb_Free(O);
		//		tb_memDump();
  }

#endif

#ifdef TEST_GC
	if(1) {
		int a, b, i, c;
		char *v;
		//		dumpSelfCheck(selfCheck(&I));
		tb_trace(TB_WARN, "Heap size: %d\n", GC_get_heap_size());
		tb_trace(TB_WARN, "free_bytes: %d\n", GC_get_free_bytes());

		bigalloc();
		//		GC_gcollect();
		dumpSelfCheck(selfCheck(&I));
		a = GC_get_heap_size();
		b = GC_get_free_bytes();
		tb_trace(TB_WARN, "Heap size: %d Free :%d (used %d)\n", a, b, a -b);
		c = a;
		for(i = 0; i<1000; i++) {
			v = malloc(1024);
			a = GC_get_heap_size();
			b = GC_get_free_bytes();
			if(a != c) {
				tb_trace(TB_WARN, "Heap size: %d Free :%d (used %d)\n", a, b, a -b);
			}
			c = a;
			//			free(v);
		}
		//		GC_gcollect();
		a = GC_get_heap_size();
		b = GC_get_free_bytes();
		tb_trace(TB_WARN, "Heap size: %d Free :%d (used %d) [%d]\n", a, b, a -b,
						 ((int)(b/65536))*65536);
		/*
		b = ((int)(b/65536))*65536;
		while( b > 65536 ) {
			sbrk( -65536 );
			//			GC_expand_hp ( -65536 );
			a = GC_get_heap_size();
			b = GC_get_free_bytes();
			tb_trace(TB_WARN, "Heap size: %d Free :%d (used %d)\n", a, b, a -b);
			dumpSelfCheck(selfCheck(&I));
		}
		*/
	}
#endif

#ifdef TEST_DICT
	if(1) {
		tb_Obj_t t = tb_New(TB_STRING, NULL);
		tb_Obj_t Dummy = tb_New(TB_STRING, NULL);
		Dict_t D = tb_new_dict(t, NULL);
		dict_insert_node(D, "un", tb_New(TB_STRING, "1"));
		dict_insert_node(D, "deux", tb_New(TB_STRING, "2"));
		dict_insert_node(D, "trois", tb_New(TB_STRING,"3"));
		dict_insert_node(D, "quatre", tb_New(TB_STRING, "4"));
		dict_insert_node(D, "cinq", tb_New(TB_STRING, "5"));
		dict_insert_node(D, "six", tb_New(TB_STRING, "6"));

		tb_Dump(dict_keys(D));
		fprintf(stderr, "1------------------------\n");
		//		tb_Dump(dict_lookup(D, "trois"));
		dict_remove(D, "un");
		tb_Dump(dict_keys(D));
		fprintf(stderr, "2------------------------\n");
		dict_remove(D, "deux");
		tb_Dump(dict_keys(D));
		fprintf(stderr, "3------------------------\n");
		dict_remove(D, "trois");
		tb_Dump(dict_keys(D));
		fprintf(stderr, "4------------------------\n");
		dict_remove(D, "quatre");
		tb_Dump(dict_keys(D));
		fprintf(stderr, "5------------------------\n");
		dict_remove(D, "cinq");
		tb_Dump(dict_keys(D));
		fprintf(stderr, "6------------------------\n");
		dict_remove(D, "six");

		tb_Dump(dict_keys(D));
		dict_insert_node(D, "quatre", tb_New(TB_STRING, "4"));
		dict_insert_node(D, "cinq", tb_New(TB_STRING, "5"));
		dict_insert_node(D, "six", tb_New(TB_STRING, "6"));
		tb_Dump(dict_keys(D));
	}

#endif

#ifdef TEST_SORT
	if(1) {
		Varray_t V = tb_New(TB_VARRAY);
		tb_tokenize(V, "un deux trois quatre cinq six sept trois huit neuf dix", " ", 1);
		tb_Dump(V);
		tb_Dump(tb_Sort(V, NULL));
		tb_Dump(tb_Reverse(V));
	}


#endif

  return 0;
}

void bigalloc(void) {
  ps_info_t I;
	void *blob = GC_malloc(1024 * 1024 * 10);
	tb_trace(TB_WARN, "big alloc done\n");
	//dumpSelfCheck(selfCheck(&I));
	tb_trace(TB_WARN, "now returning\n");
}

void doIt(void *vc, void *data) {
  int i;
	String_t S = (String_t)vc;

  for(i = tb_getSize(S); i >= 0; i--) {
    printf("%c%s", (char)(S2sz(S))[i],  (char *)data);
  }
  printf("\n");
}

int search(void *vc, void *data) {
	String_t S = (String_t)vc;
  if(strstr(S2sz(S), (char *)data)) return 1;
  return 0;
}

int callback(void *arg) {
	tb_Obj_t S = (tb_Obj_t)arg;
	char buff[MAX_BUFFER+1];

	tb_trace(TB_WARN, "Server: New cx \n");
	tb_Dump(S);
	tb_readSock(S, buff, MAX_BUFFER);

	tb_trace(TB_WARN, "Server: recv<%s>\n", buff);
	tb_writeSock(S, "BYE");
	tb_trace(TB_WARN, "Server: exit callback\n");
	tb_Free(S);

	return TB_OK;

}















