//------------------------------------------------------------------
// $Id: String.c,v 1.4 2004/06/15 15:08:27 plg Exp $
//------------------------------------------------------------------
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

/**
 * @file String.c 
 */

/**
 * @defgroup String String_t
 * @ingroup Scalar
 * String related methods and functions
 */

#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "Toolbox.h"
#include "tb_global.h"
#include "strfmt.h"
#include "String.h"
#include "tb_ClassBuilder.h"
#include "C_Castable_interface.h"
#include "Serialisable_interface.h"

#include "Memory.h"
#include "Error.h"


inline string_members_t XStr(String_t This) {
	return (string_members_t)((__members_t)tb_getMembers(This, TB_STRING))->instance;
}

static void       * tb_string_free          (String_t S);
static String_t     tb_string_clone         (String_t S);
static String_t     tb_string_clear         (String_t S);
static void         tb_string_dump          (String_t S, int level);
static int          tb_string_getsize       (String_t S);
static String_t     tb_string_stringify     (String_t S);
static cmp_retval_t tb_string_compare       (String_t S1, String_t S2);

void __build_string_once(int OID) {
	tb_registerMethod(OID, OM_NEW,          tb_string_new);
	tb_registerMethod(OID, OM_FREE,         tb_string_free);
	tb_registerMethod(OID, OM_GETSIZE,      tb_string_getsize);
	tb_registerMethod(OID, OM_CLONE,        tb_string_clone);
	tb_registerMethod(OID, OM_DUMP,         tb_string_dump);
	tb_registerMethod(OID, OM_CLEAR,        tb_string_clear);
	tb_registerMethod(OID, OM_COMPARE,      tb_string_compare);

	//	tb_registerMethod(OID, OM_STRINGIFY,    S2sz); // fixme: should be escaped ("\"")
	tb_registerMethod(OID, OM_STRINGIFY,    tb_string_stringify);

	tb_implementsInterface(OID, "C_Castable", 
												 &__c_castable_build_once, build_c_castable_once);
	
	tb_registerMethod(OID, OM_TOSTRING,     S2sz);
	tb_registerMethod(OID, OM_TOINT,        S2int);
	tb_registerMethod(OID, OM_TOPTR,        S2sz);

	tb_implementsInterface(OID, "Serialisable", 
												 &__serialisable_build_once, build_serialisable_once);

	tb_registerMethod(OID, OM_MARSHALL,     tb_string_marshall);
	tb_registerMethod(OID, OM_UNMARSHALL,   tb_string_unmarshall);
}



String_t dbg_tb_string_new(char *func, char *file, int line, int len, char *data);





/**	String_t to char * typecaster
 *
 * Allows direct access to char * internal. Use like a read-only value.
 *
 * @param S : String_t to cast
 * @return internal char * string
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * \ingroup String
 */
inline char *S2sz(String_t S) {
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		return (char *)XStr(S)->data;
	}
	return NULL;
}



static String_t tb_string_stringify(String_t S) {
	return tb_StrQuote(tb_Clone(S));
}

String_t tb_StrQuote(String_t S) {
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		tb_Sed("\"", "\\\"", S, PCRE_MULTI|PCRE_DOTALL);
		tb_StrAdd(S, 0, "\"");
		tb_StrAdd(S, -1, "\"");
		return S;
	}
	return NULL;
}

String_t tb_StrUnQuote(String_t S) {
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		tb_Sed("^\"", "", S, PCRE_MULTI|PCRE_DOTALL);
		tb_Sed("\"$", "", S, PCRE_MULTI|PCRE_DOTALL);

		tb_Sed("\\\\\"", "\"", S, PCRE_MULTI|PCRE_DOTALL);
		return S;
	}
	return NULL;
}




/**	String_t to int typecaster
 *
 * Returns string internal as integer (via atol)
 *
 * @param S : String_t to cast
 * @return int value
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * \ingroup String
 */
inline int S2int(String_t S) {
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		return atol((char *)XStr(S)->data);
	}
	return TB_ERR;
}


String_t dbg_tb_string(char *func, char *file, int line, char *fmt, ...) {
	char *s;
	String_t S;
	int len;

	if(fmt) {
		va_list parms;
		va_start(parms, fmt);

		len = __tb_vasprintf(&s, fmt, parms);
		S = dbg_tb_string_new(func, file, line, len, s);
		tb_xfree(s);
	} else {
		S = dbg_tb_string_new(func, file, line, 0, NULL);
	}

	return S;
}



String_t dbg_tb_nstring(char *func, char* file, int line, int len, char *fmt, ...) {
	char *s;
	String_t S;
	int rlen;

	if(fmt) {
		va_list parms;
		va_start(parms, fmt);

		rlen = __tb_vnasprintf(&s, len, fmt, parms);
		S = dbg_tb_string_new(func, file, line, rlen, s);
		tb_xfree(s);
	} else {
		S = dbg_tb_string_new(func, file, line, 0, NULL);
	}

	return S;
}



String_t dbg_tb_string_new(char *func, char *file, int line, int len, char *data) {
	set_tb_mdbg(func, file, line);
	return tb_string_new(len , data);
}


/**	String_t constructor
 *
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 *
 * Methods:
 * - tb_StrUcase : string to upper
 * - tb_StrLcase : string to lower
 * - tb_StrAdd : concat strings
 * - tb_StrnAdd : concat string with max size
 * - tb_RawAdd : concat binary data to string
 * - tb_StrDel : remove string part 
 * - tb_StrGet : extract string part 
 * - tb_StrSub : copy string part
 * - tb_StrRepl : replace string part
 * - tb_Join : concat array of strings
 *
 * @param fmt : format
 * @param ... : format's args
 * @return newly allocated String_t
 * 
 * @see Object, Scalar
 * \ingroup String
 */
String_t tb_String(char *fmt, ...) {
	char *s;
	String_t S;
	int len;

	if(fmt) {
		va_list parms;
		va_start(parms, fmt);
		
		len = __tb_vasprintf(&s, fmt, parms);
		S = tb_string_new(len, s);
		tb_xfree(s);
	} else {
		S = tb_string_new(0, NULL);
	}

	return S;
}


/** String_t constructor with max size
 *
 * Allows checking of max lenght for newly created string.
 * format is fully printf compliant (see stdio.h). Toolbox extends standard formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 * Example: 
 * \code
 * String_t S = tb_New_nString(50, "hello %s %d", "world", 1);
 * \endcode
 * @param len : max len of expanded format
 * @param fmt : formatting pattern
 * @param ... : variadic args
 * @return newly allocated String_t
 * @see String, Object, Scalar
 * @ingroup String
*/
String_t tb_nString(int len, char *fmt, ...) {
	char *s;
	String_t S;
	int rlen;

	if(fmt) {
		va_list parms;
		va_start(parms, fmt);

		rlen = __tb_vnasprintf(&s, len, fmt, parms);
		S = tb_string_new(rlen, s);
		tb_xfree(s);
	} else {
		S = tb_string_new(0, NULL);
	}

	return S;
}


/**	(yet another) String_t constructor
 *
 * data may contains '%s' or any formatting token, it won't be expanded.
 * For formatting constructor see tb_String and tb_nString
 *
 * @param len : length of data to be copied
 * @param data : pointer to source data
 * @return newly allocated String_t
 *
 * @see String, Object, Scalar
 * \ingroup String
 */
String_t tb_string_new(int len, char *data) {
	tb_Object_t This;
	string_members_t m;
	pthread_once(&__class_registry_init_once, tb_classRegisterInit);
	
	This = tb_newParent(TB_STRING);

	This->isA     = TB_STRING;
	This->refcnt  = 1;
	This->members->instance  = (string_members_t)tb_xcalloc(1, sizeof(struct string_members));
	m = This->members->instance;
	m->size        = len;
	m->grow_factor = 2;
	
	if(len) {
		m->data = tb_xcalloc(1, len +1);
		memcpy(m->data, data, len);
		m->allocated = len+1;
	} else {
		m->data = tb_xcalloc(1, 1);
		m->allocated = 1;
	}
	if(fm->dbg) fm_addObject(This);

	//	tb_warn("tb_string_new: instance_type=%d\n", This->members->instance_type);

	return This;
}



void tb_string_marshall(String_t marshalled, String_t S, int level) {
	char indent[level+1];
	string_members_t m = XStr(S);
	memset(indent, ' ', level);
	indent[level] = 0;
	
	if(m->size >0) {
		tb_StrAdd(marshalled, -1, "%s<string>%s</string>\n", indent, (char *)m->data);
	} else {
		tb_StrAdd(marshalled, -1, "%s<string/>\n", indent);
	}
}


String_t tb_string_unmarshall(XmlElt_t xml_entity) {
	String_t S;
	Vector_t V;
	if(! streq(S2sz(XELT_getName(xml_entity)), "string")) {
		tb_error("tb_string_unmarshall: not a string Elmt\n");
		return NULL;
	}
	tb_debug("in tb_String unmarshaller\n");
	if((V = XELT_getChildren(xml_entity)) != NULL &&
		 tb_getSize(V) >0) {
		S = tb_String("%S", XELT_getText(tb_Get(V, 0)));
	} else {
		S = tb_String(NULL);
	}
	return S;
}


static int tb_string_getsize(String_t S) {
	return XStr(S)->size;
}

static void *tb_string_free(String_t S) {
	string_members_t m = XStr(S);
	if(m && m->data) tb_xfree(m->data);
	tb_freeMembers(S);
	S->isA = TB_STRING;
	return tb_getParentMethod(S, OM_FREE);
}

static void tb_string_dump(String_t S, int level) {
	int i;
	string_members_t m = XStr(S);
  for(i = 0; i<level; i++) fprintf(stderr, " ");
	fprintf(stderr, "<TB_STRING SIZE=\"%d\" ALLOCATED=\"%d\" ADDR=\"%p\" DATA=\"%p\" REFCNT=\"%d\" DOCKED=\"%d\" ",
					m->size, m->allocated, S, m->data, S->refcnt, S->docked);
	if(m->size >0) {
		fprintf(stderr, ">\n%s\n", (char *)m->data);
		for(i = 0; i<level; i++) fprintf(stderr, " ");
		fprintf(stderr, "</TB_STRING>\n");
	} else {
		fprintf(stderr, "/>\n");
	}
}


static String_t tb_string_clone(String_t S) {
	return tb_String("%s", XStr(S)->data); // fixme: optimizable
}

static String_t tb_string_clear(String_t S) {
	string_members_t m = XStr(S);
	tb_xfree(m->data);
	m->data = tb_xcalloc(1,1);
	m->allocated = 1;
	m->size = 0;

	return S;
}


inline int strsz(char *s) { return (s) ? strlen(s) +1 : -1 ; }

int dbg_tb_tokenize(char *func,char *file, int line, Vector_t V, char *s, char *delim, int flags) {
	set_tb_mdbg(func, file, line);

	return tb_tokenize(V, s, delim, flags);
}

/** Split a String_t in tokens, and push them in Vector_t
 *
 * Splitting is done using any char from 'delim' string. Tokenisation doesn't modify 
 * source String_t. 
 * 
 * Flags:
 *  - TK_KEEP_BLANKS : store empty tokens (if two adjacent delimiters found)
 *  - TB_ESC_QUOTES : don't split inside quoted string
 * 
 * Example:
 * \code
 * Vector_t V = tb_Vector();
 * char s[] = "he says : \"goodbye cruel world\" and dies";
 * int rc = tb_tokenize(V, s, " ", TK_ESC_QUOTES);
 * 
 * V will contains {'he', 'says', ':', 'goodbye cruel world', 'and', 'dies'}
 * \endcode
 *
 * @param V : Vector_t holding the result set
 * @param s : subject string
 * @param delim : string containing all delimiters (every char in string is individually a delimiter)
 * @param flags : special operation modes (or-able values)
 * - TK_KEEP_BLANKS : add an empty String_t in result set when two delimiters are stucked
 * - TK_ESC_QUOTES : skip tokenization inside quotes ( "..." or '...')
 *
 * @return number of tokens
 * @warning 
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VECTOR
 * - TB_ERR_BAD if target string is NULL
 * - TB_ERR_UNSET if delimiter string is NULL
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
int tb_tokenize(Vector_t V, char *s, char *delim, int flags) {
  char *p, *st, *d, *q;
  int len       = 0;
  int f         = 0;
	int no_blanks = !(flags  & TK_KEEP_BLANKS);
	char *quotes  = NULL;
	no_error;
	if(flags & TK_ESC_QUOTES) quotes = "\"'";

  if(s == NULL) { 
		tb_error("tb_tokenize: can't split NULL !!\n"); 
		set_tb_errno(TB_ERR_BAD);
		return TB_ERR; 
	}
  if(delim == NULL) { 
		tb_error("tb_tokenize: can't split on NULL !!\n"); 
		set_tb_errno(TB_ERR_UNSET);
		return TB_ERR; 
	}
  if(! tb_valid(V, TB_VECTOR, __FUNCTION__)) 	return TB_ERR; 
	

  for(st = p = s; *p != '\0'; len++) {
    d = delim;
    while(*d != '\0') {

			if(quotes) {
				q = quotes;
				while(*q != '\0') {
					if(*q == *p) {
						p++;
						len++;
						while(*p && *p != *q) { p++; len++; }
						if(*p) p++;
						len++;
						break;
					}
					q++;
				}
			}

      if(*d == *p) {
				if(no_blanks) {
					if(len >0) {
						String_t Z;
						tb_Push(V, Z = tb_string_new(len, st));
						
					}
				} else {
					tb_Push(V, tb_string_new(len, st));
				}
				len = -1;
				p++;
				st = p;
				f = 1;
				break;
      }
      d++;
      f=0;
    }
    if(! f)  p++;
  }
  if(no_blanks) {
    if(len >0) {
			tb_Push(V, tb_string_new(len, st));
    }
  } else {
		tb_Push(V, tb_string_new(len, st));
  }

  return tb_getSize(V);
} 

Vector_t dbg_tb_strsplit(char *func, char *file, int line, char *str, char *regex, int options) {
	set_tb_mdbg(func, file, line);

	return tb_StrSplit(str, regex, options);
}

/** Split a String_t in tokens using a regex delimiter, and push then in Vector_t
 *
 * Splitting is done in two passes : first the regex is searched and if found converted in
 * 0x01 char. then tb_tokenize is called to operate real splitting. Source string is not mangled 
 * (replacement is done on a string copy).
 *
 * Flags:
 *
 * Example:
 * \code
 * Vector_t V = tb_Vector();
 * char s[] = "goodbye cruel world";
 * int rc = tb_StrSplit(s, "(c\\w+l)", 0);
 * 
 * V will contains {'goodbye ', ' world'}
 * \endcode
 *
 * for other examples with regexes see tb_Sed, tb_matchRegex, tb_getRegex
 *
 * @param str : subject string
 * @param regex : delimiter regex pattern
 * @param options : special regex flags :
 * - PCRE_CASELESS        : case insensitive
 * - PCRE_MULTILINE       : pattern may run over lines breaks (^/$ applies on every lines)
 * - PCRE_DOTALL          : match even LF
 * - PCRE_EXTENDED        : not used
 * - PCRE_ANCHORED        : '^' match only start of buffer
 * - PCRE_DOLLAR_ENDONLY  : '$' match only end of buffer
 * - PCRE_UNGREEDY        : toggle greediness to off (default is greedy)
 * - PCRE_NOBLANKS        : toolbox misc add : skip empty backrefs
 * - PCRE_MATCHMULTI      : toolbox misc add : apply pattern more than once if possible
 * - PCRE_MULTI           : toolbox misc add : same as PCRE_MATCHMULTI
 * (All flags come from the pcre library (Philip Hazel <ph10@cam.ac.uk>). This lib is used
 * internally by Toolbox for all regex operations. For extensive documentation see pcre(7) )
 *
 * @return: new Vector_t of tokens or NULL if str or regex is NULL
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
Vector_t tb_StrSplit(char *str, char *regex, int options) {
	char delim[2] = { 0x01, 0 };
	String_t S, R;
	Vector_t V;

  if(str == NULL)  { 
		tb_error("tb_strSplit: can't split NULL !!\n"); 
		return NULL; 
	}
  if(regex == NULL) { 
		tb_error("tb_strSplit: can't split on NULL !!\n"); 
		return NULL; 
	}

	S = tb_String("%s", str);
	R = tb_String("%s", regex);

	if(! tb_matchRegex(R, "^\\(.*\\)$", 0)){ 
		tb_Sed(".*", "($1)", R, 0);
	}
	tb_Sed(XStr(R)->data, delim, S, options); 

	tb_tokenize(V = tb_Vector(), XStr(S)->data, delim, options);
	tb_Free(S);
	tb_Free(R);

  return V;
}

String_t dbg_tb_join(char *func, char *file, int line, Vector_t V, char *delim) {
	set_tb_mdbg(func, file, line);

	return tb_Join(V, delim);
}


/** Concatenate String_t array into a new String_t, separated by delim string
 *
 * Create an  new String_t object with Vector_t's String_t contents separated by 'delim' string
 *
 * Example:
 * \code
 * Vector_t V = tb_Vector();
 * String_t S;
 * 
 * tb_Push(V, "");
 * tb_Push(V, "usr");
 * tb_Push(V, "local");
 * tb_Push(V, "include");
 * tb_Push(V, "Toolbox.h");
 * 
 * S = tb_Join(V, "/");
 * 
 * S will contains "/usr/local/include/Toolbox"
 *\endcode
 * 
 * @param V : Vector containing strings to concatenate
 * @param delim : string to glue between Vector's contents
 * @returns: new String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if V not a TB_VARRAY
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_Join(Vector_t V, char *delim) {
  String_t out = tb_String(NULL);
  int maxl, i;

  if(tb_valid(V, TB_VECTOR, __FUNCTION__)) {
	
		if((maxl = tb_getSize(V)) == 0) return out;

		if(tb_valid(tb_Get(V, (tb_Key_t)0), TB_STRING, __FUNCTION__)) {
			tb_StrAdd(out, -1, "%s", tb_toStr(V, (tb_Key_t)0));
		}

		for(i = 1; i < maxl ; i++) {

			tb_StrAdd(out, -1, "%s", delim);

			if(tb_valid(tb_Get(V, (tb_Key_t)i), TB_STRING, __FUNCTION__)) {
				tb_StrAdd(out, -1, "%s", tb_toStr(V, (tb_Key_t)i));
			}
		}

		return out;
	}
	return V; 
}


/* deprecated
char * stripnl(char *str) {
  char *s;
  if((s = strchr(str, '\n')) != NULL) *s = '\0';
  return str;
}
*/

/** String_t to upper case
 *
 * Convert string to upper case
 * 
 * @param S : target String_t
 * @returns: modified String_t
 * 
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrUcase(String_t S) {
	int max, i;
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		string_members_t m = XStr(S);
		max = tb_getSize(S);
		if(max) {
			for(i = 0; i < max; i++) {
				((char *)m->data)[i] = toupper(((unsigned char *)m->data)[i]);
			}
		}
	}
	return S; 
}

/** String_t to lower case
 *
 * Convert string to upper case
 * 
 * @param S : target String_t
 * @returns: modified String_t
 * 
 * ERROR:
 * in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * 
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrLcase(String_t S) {
	int              max, i;
	
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		string_members_t m = XStr(S);
		max = tb_getSize(S);
		if(max) {
			for(i = 0; i < max; i++) {
				((char *)m->data)[i] = tolower(((unsigned char *)m->data)[i]);
			}
		}
	}
	return S;
}



String_t dbg_tb_substr(char *func, char *file, int line, String_t S, int start, int len) {
	set_tb_mdbg(func, file, line);

	return tb_StrSub(S, start, len);
}


/** Copy a String_t part into a new String_t
 *
 * Copy len chars from String_t into a new String_t, at offset start.
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 * Len arg value of '-1' is allowed, meaning 'remaining of source string'
 *
 * Example:
 * \code
 * String_t tst = tb_String("hello world");
 * String_t S = tb_StrSub(tst, -5, -1);
 * will create a String_t S containing "world".
 * \endcode
 *
 *
 * @return: new String_t
 *
 * @warning 
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrSub(String_t S, int start, int len) {
	String_t new;

	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		string_members_t m = XStr(S);

		if(len == 0 || (TB_ABS(start) > m->size)) {
			tb_error("tb_StrSub: args out of bounds\n");
			return NULL;
		}

		if(start < 0) { // extract string starting from end
			start = (m->size - TB_ABS(start));
		}

		if(len == -1) { // all remaining chars
			new = tb_string_new(m->size - start, m->data + start);
		} else {
			len = TB_MIN(len, (m->size - start));
			new = tb_string_new(len, m->data + start);
		}
		
		return new;
	}

	return NULL;
}


/** Extract a String_t part into a new String_t (extracted fragment is removed from source)
 *
 * Take out len chars from String_t into a new String_t, at offset start.
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 * Len arg value of '-1' is allowed, meaning 'remaining of source string'
 *
 * Example:
 * \code
 * String_t S = tb_StrSub(tb_String("hello world"), -5, -1);
 * will create a String_t containing "world".
 * \endcode
 *
 *
 * @return: new String_t
 *
 * @warning 
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrGet(String_t S, int start, int len) {
	String_t new;

	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__)) { 
		string_members_t m = XStr(S);

		if(len == 0 || (TB_ABS(start) > m->size)) {
			tb_error("tb_StrGet: args out of bounds\n");
			return NULL;
		}

		if(start < 0) { // extract string starting from end
			start = (m->size - TB_ABS(start));
		}

		if(len == -1) { // all remaining chars
			new = tb_string_new(m->size - start, m->data + start);
		} else {
			len = TB_MIN(len, (m->size - start));
			new = tb_string_new(len, m->data + start);
		}
		tb_StrDel(S, start, len); // not optimal, but probably acceptable

		return new;
	}
	return NULL;
}


inline static int str_check_bounds(int size, int *start, int *len) {
	if(*start < 0) { // starting from end
		*start = (size - TB_ABS(*start));
		if(*start >= size || *start < 0) 	return 0;
	}

	if(*len == -1) { // == from start to end
		*len = size - *start;
	} else if(*len < -1) {
		return 0; // len limits are [-1 .. size]
	}

	if(*start+ *len > size || *start+ *len <0) return 0;

	return 1;
}


/** tb_StrDel - Remove a String_t part
 *
 * Remove len chars from String_t, at offset start.
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 *
 * @param S : traget String_t
 * @param start : string offset (negative for reverse count)
 * @param len : lenght of substring (-1 for all chars to end of string)
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrDel(String_t S, int start, int len) {
	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {

		string_members_t m = XStr(S);

		if(len == 0) return S;

		if(! str_check_bounds(m->size, &start, &len)) {
			tb_error("tb_StrDel: args out of bounds (%d->%d/%d)\n", start, len, m->size);
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return NULL;
		}

		m->size -= len;
		if(m->size == 0) { // wipeout full string
			m->data = tb_xrealloc(m->data, 1);
			m->allocated =1;
			*(char *)(m->data) = 0;

		} else {
			memmove(m->data + start, m->data + start + len, m->size - start);

			((char *)m->data)[m->size] = 0;

			if(m->allocated - m->size >= m->size*2) {
				char *s;
				m->allocated = m->size*2;
				s = tb_xrealloc(m->data, m->allocated);
				// if s == NULL :-> die horribly
				m->data = s;
				tb_debug("(strdel) reallocated %p -> %d/%d\n", S, m->size, m->allocated);
			} else {
				tb_debug("(strdel) no realloc needed %p (%d/%d)\n", S, m->size, m->allocated);
			}
		}

		return S;
	}

	return NULL;
}




/** Copy formatted string into String_t
 *
 * Insert into target String_t resulting expansion of fmt, starting at 'start' offset.
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 *
 * format:
 *
 * format is fully printf compliant (see stdio.h). Toolbox extends standard
 * formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 * @param S : target String_t
 * @param start : insert offset (use -1 to add at end of existing string)
 * @param fmt : same as printf formating patterns
 * @param ... : expension parameters matching for pattern symbols
 * @return: target String_t
 *
 * Example:
 * \code
 * String_t S = tb_String("Goodbye");
 * tb_StrAdd(S, -1, "%s !", "world");
 * will give "Goodbye world !"
 * tb_StrAdd(S, 7, " cruel");
 * will give "Goodbye cruel world !"
 * \endcode
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrAdd(String_t S, int start, char *fmt, ...) {
	int               len;
	char            * s;
	va_list parms;

	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
	
		string_members_t m = XStr(S);

		if(fmt == NULL) return S;

		if(m->size == 0 && start < 0) start = 0;

		if(!(TB_ABS(start) <= m->size)) {
			tb_error("tb_StrAdd: unbound start (%d) sz=%d",
							 start, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return S;
		}

		va_start(parms, fmt);
		__tb_vasprintf(&s, fmt, parms);

		if((len = strlen(s)) == 0) {
			tb_xfree(s);
			return S;
		}

		if(start < 0) { // add string starting from end
			start = (m->size - TB_ABS(start)) +1;
		}

		if(m->allocated <= (m->size + len +1)) {
			m->allocated = (m->size + len +1) * m->grow_factor;
			m->data = tb_xrealloc(m->data, m->allocated);
			tb_debug("(stradd) reallocated %p -> %d gf=%d\n", 
							 S, m->allocated, m->grow_factor);
		} else {
			tb_debug("(stradd) no realloc needed %p (%d/%d)\n", 
							 S, m->size, m->allocated);
		}
		if(start < m->size) {
			memmove(m->data + start + len, m->data + start, (m->size - start) + 1);
		}

		memmove(m->data + start, s, len);
		m->size += len;
		((char *)m->data)[m->size] = 0;

		tb_xfree(s);

		return S;
	}
	return NULL;
}


/** Copy formatted string into String_t with limit on expansion size
 *
 * Insert into target String_t resulting expansion of fmt (at most len char wide), starting at 'start' offset.
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 *
 * format:
 *
 * format is fully printf compliant (see stdio.h). Toolbox extends standard
 * formating elements with tokens '%S' for String_t and '%D' for Num_t. 
 *
 * @param S : target String_t
 * @param len : maximum expansion size for fmt
 * @param start : insert offset (use -1 to add at end of existing string)
 * @param fmt : same as printf formating patterns
 * @param ... : expension parameters matching for pattern symbols
 * @return: target String_t
 *
 * Example:
 * \code
 * String_t S = tb_String("Goodbye");
 * tb_StrAdd(S, -1, "%s !", "world");
 * will give "Goodbye world !"
 * tb_StrAdd(S, 7, " cruel");
 * will give "Goodbye cruel world !"
 * \endcode
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrnAdd(String_t S, int len, int start, char *fmt, ...) {
	char           * s;
	int              nlen;
	string_members_t   m;
	va_list parms;

	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
	
		if(fmt == NULL || len == 0) return S;
		m = XStr(S);

		if(m->size == 0 && start < 0) start = 0;

		if(!(TB_ABS(start) <= m->size)) {
			tb_error("tb_StrnAdd: unbound start (%d) sz=%d",
							 start, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return S;
		}
		va_start(parms, fmt);
		__tb_vnasprintf(&s, len+1, fmt, parms);

		if((nlen = strlen(s)) == 0) return S;
		if(start < 0) { // add string starting from end
			start = (m->size - TB_ABS(start)) +1;
		}

		if(m->allocated <= (m->size + nlen +1)) {
			char *ts;
			m->allocated = (m->size + nlen +1) * m->grow_factor;
			ts = tb_xrealloc(m->data, m->allocated);
			m->data = ts;
			tb_debug("(strnadd) reallocated %p -> %d\n", S, m->allocated);
		} else {
			tb_debug("(strnadd) no realloc needed %p (%d/%d)\n", S, m->size, m->allocated);
		}

		memmove(m->data + start + nlen, m->data + start, (m->size - start) + 1);
		memcpy(m->data + start, s, nlen);
		m->size += nlen;
		((char *)m->data)[m->size] = 0;
		tb_xfree(s);
	}
	return S;
}

/** Copy binary data into String_t
 *
 * Insert into target String_t binary data (can contains 0x00 bytes).
 * This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 *
 *
 * @param S : target String_t
 * @param len : data size
 * @param start : insert offset (use -1 to add at end of existing string)
 * @param raw : pointer to data to insert
 * @return: target String_t
 *
 * @warning:
 * Raw_t is a better choice for storing binary
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_RawAdd(String_t S, int len, int start, char *raw) {
	string_members_t m;

	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
	
		if(raw == NULL || len == 0) return S;
		m = XStr(S);

		if(m->size == 0 && start < 0) start = 0;

		if(!(TB_ABS(start) <= m->size)) {
			tb_error("tb_RawAdd: unbound start (%d) sz=%d",
							 start, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return S;
		}

		if(start < 0) { // add string starting from end
			start = (m->size - TB_ABS(start)) +1;
		}

		if(m->allocated <= (m->size + len +1)) {
			char *s;
			m->allocated = (m->size + len +1) * m->grow_factor;
			s = tb_xrealloc(m->data, m->allocated);
			m->data = s;
			tb_debug("(rawadd) reallocated %p -> %d\n", S, m->allocated);
		} else {
			tb_debug("(rawadd) no realloc needed %p (%d/%d)\n", S, m->size, m->allocated);
		}

		memmove(m->data + start + len, m->data + start, (m->size - start) + 1);
		memcpy(m->data + start, raw, len);
		m->size += len;
		((char *)m->data)[m->size] = 0;

	}
	return S;
}

/** Expand String_t with a filler char
 *
 * Insert into target String_t 'len' bytes of 'filler' char.
 * Insertion begin at 'start' offset. This offset may be negative, meaning 'count from end'
 * (last char is at offset -1). Bound checking is done.
 *
 * @param S : target String_t
 * @param len : filler repetition factor
 * @param start : insert offset (use -1 to add at end of existing string)
 * @param filler : char to insert
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 * - TB_ERR_OUT_OF_BOUNDS  if start out of string limits
 *
 * @see: String_t, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
String_t tb_StrFill(String_t S, int len, int start, char filler) {
	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		string_members_t m = XStr(S);

		if(m->size == 0 && start < 0) start = 0;

		if(!(TB_ABS(start) <= m->size)) {
			tb_error("tb_StrFill: unbound start (%d) sz=%d",
							 start, m->size); 
			set_tb_errno(TB_ERR_OUT_OF_BOUNDS);
			return S;
		}
	
		if(len <= 0) return S;

		if(start < 0) { // add string starting from end
			start = (m->size - TB_ABS(start)) +1;
		}

		if(m->allocated <= (m->size + len +1)) {
			char *s;
			m->allocated = (m->size + len +1) * m->grow_factor;
			s = tb_xrealloc(m->data, m->allocated);
			m->data = s;
			tb_debug("(strfill) reallocated %p -> %d\n", S, m->allocated);
		} else {
			tb_debug("(strfill) no realloc needed %p (%d/%d)\n", S, m->size, m->allocated);
		}

		memmove(m->data + start + len, m->data + start, (m->size - start) + 1);
		memset(m->data + start, filler, len);
		m->size += len;
		((char *)m->data)[m->size] = 0;

	}
	return S;
}

/** Replace C string 'search' with 'replace' in String_t S.
 *
 * Searches flat pattern in target. If search pattern is found, it will be replaced by replace string, else
 * nothing is done. Replacement is done only once.
 *
 * @param S : target String_t
 * @param search : string to search
 * @param replace : replacement string
 * @return: RETCODE
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String_t, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
retcode_t tb_StrRepl(String_t S, char *search, char *replace) {
  char           * t;
  int              start, len;
  int              lsearch, lreplace;

  no_error;

  if(tb_valid(S, TB_STRING, __FUNCTION__) && search != NULL && replace != NULL ) {    
  
		string_members_t m = XStr(S);

		len = strlen(search);
		lsearch = len;
		lreplace = strlen(replace);

		if((t = strstr(m->data, search)) != NULL) {
			start = (int)((int)t - (int)m->data);

			if(lsearch == lreplace) {
				memcpy(m->data + start, replace, lreplace);

			} else if(lsearch > lreplace) {
				memcpy(m->data + start, replace, lreplace);
				memmove(m->data + start + lreplace,
								m->data + start + lsearch,
								m->size - (start + lsearch) +1); //  +1 = '\0' terminal

				m->size -= lsearch - lreplace;

				/* Too big => reducing */
				if(m->allocated >= (m->size * 3/*XSTR(S)->shrink_factor*/)) {    
					char *s;
					m->allocated = (m->size + 1) * m->grow_factor;
					s = tb_xrealloc(m->data, m->allocated);
					if(s != NULL) {
						m->data = s;
					}
				}


			} else { // lsearch < lreplace
				/* not enough size => growing */
				int oldsize = m->size;
				m->size += lreplace - lsearch;

				if(m->allocated < m->size + 1) {
					char *s;
					m->allocated = (m->size + 1) * m->grow_factor;
					s = tb_xrealloc(m->data, m->allocated);
					// if s == NULL :-> die horribly
					m->data = s;
				}

				memmove(m->data + start + lreplace,
								m->data + start + lsearch,
								oldsize - (start + lsearch) +1); //  +1 = '\0' terminal
				memcpy(m->data + start, replace, lreplace);
			}
    
			return TB_OK;
		}

		return TB_KO;
	}
	return TB_ERR;
}


/** Compare String_t with a char *string
 *
 * Shortcut for a strcmp on String_t internal C string
 *
 * @param S : target String_t
 * @param match : string to compare with
 * @param case_sensitive : 1 for case sensitive compare, else case insensitive
 * @return: integer less than, equal  to,  or greater  than  zero if String_t is found, respectively, to be less than, to match, or be greater than 'match'.
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
int tb_StrCmp(String_t S, char *match, int case_sensitive) {
	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__) && match != NULL) {
		string_members_t m = XStr(S);
		
		if(case_sensitive) {
			return strcasecmp(m->data, match);
		} 

		return strcmp(m->data, match);
	}
 return TB_ERR;
}

static cmp_retval_t tb_string_compare(String_t S1, String_t S2) {
	string_members_t m1 = XStr(S1);
	string_members_t m2 = XStr(S2);
	int rez = strcmp(m1->data, m2->data);
	if(rez == 0) return TB_CMP_IS_EQUAL;
	return (rez >0) ? TB_CMP_IS_GREATER : TB_CMP_IS_LOWER;
}


/** Check if String_t equals char *string with strict case sensivity
 *
 * @param S : target String_t
 * @param match : string to compare with
 * @return: TB_OK for equality, TB_KO if not equal, TB_ERR if not a String_t parameter
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
retcode_t tb_StrEQ(String_t S, char *match) {
	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__) && match != NULL) {
		string_members_t m = XStr(S);
		
		return (strcmp(m->data, match) == 0) ? TB_OK : TB_KO;
	}
 return TB_ERR;
}

/** Check if String_t equals char *string without case sensivity
 *
 * @param S : target String_t
 * @param insensitive_match : string to compare with
 * @return: TB_OK for equality, TB_KO if not equal, TB_ERR if not a String_t parameter
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
 */
retcode_t tb_StrEQi(String_t S, char *insensitive_match) {
	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__) && insensitive_match != NULL) {
		string_members_t m = XStr(S);

		return (strcasecmp(m->data, insensitive_match) == 0) ? TB_OK : TB_KO;
	}
 return TB_ERR;
}


/* Eats left spaces
 *
 * Removes spaces at begining of string (if any)
 *
 * @param S : target String_t
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
*/
String_t tb_ltrim(String_t S) { 
	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__)) 
		{
			string_members_t m = XStr(S);
			if(m->data) tb_Sed("^\\s+", "", S, 0);
		}
	return S;
}


/* Eats right spaces
 *
 * Removes trailling spaces of string (if any)
 *
 * @param S : target String_t
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
*/
String_t tb_rtrim(String_t S) {
	no_error;
	if(tb_valid(S, TB_STRING, __FUNCTION__)) 
		{
			string_members_t m = XStr(S);
			if(m->data) tb_Sed("\\s+$", "", S, 0);
		}
	return S;
}

/* Eats right and left spaces
 *
 * Removes leading and trailling spaces of string (if any)
 *
 * @param S : target String_t
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
*/
String_t tb_trim(String_t S) {
	string_members_t m = XStr(S);
	return (m && m->data) ? tb_ltrim(tb_rtrim(S)) : S;
}


/* Eats trailing line feed
 *
 * Removes line feed at end of string (if any)
 * @param S : target String_t
 * @return: target String_t
 *
 * @warning
 * ERROR: in case of error tb_errno will be set to :
 * - TB_ERR_INVALID_OBJECT if S not a TB_STRING
 *
 * @see: String, tb_StrCmp, tb_StrEQ, tb_StrEQi, tb_StrUcase, tb_StrLcase, tb_StrFill, tb_StrAdd, tb_StrnAdd, tb_RawAdd, tb_StrDel, tb_StrSub, tb_StrRepl, tb_Join, tb_tokenize
 * @ingroup String
*/
String_t tb_chomp(String_t S) {
	no_error;

	if(tb_valid(S, TB_STRING, __FUNCTION__)) {
		string_members_t m = XStr(S);

		if(m->data && ((char *)m->data)[m->size -1] == '\n') {
			if(((char *)m->data)[m->size -2] == 0x0D) {
				tb_StrDel(S, -2, 2);
			} else {
				tb_StrDel(S, -1, 1);
			}
		}
	}
	return S;
}



/*
Todo:

tb_Find(Str, str_pattern)
*/







