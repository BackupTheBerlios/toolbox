//========================================================================
// 	$Id: regex.c,v 1.1 2004/05/12 22:04:52 plg Exp $
//========================================================================
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
 * @file regex.c pcre lib interface
 */

/**
 * @defgroup Regex Regular expressions functions
 * Philip Hazel's pcre wrapper interface
 *
 *\code
*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************
*
This is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language. See
the file Tech.Notes for some information on the internals.

Written by: Philip Hazel <ph10@cam.ac.uk>

           Copyright (c) 1997-1999 University of Cambridge

-----------------------------------------------------------------------------
Permission is granted to anyone to use this software for any purpose on any
computer system, and to redistribute it freely, subject to the following
restrictions:

1. This software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

2. The origin of this software must not be misrepresented, either by
   explicit claim or by omission.

3. Altered versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

4. If PCRE is embedded in any software that is released under the GNU
   General Purpose Licence (GPL), then the terms of that licence shall
   supersede any condition above with which it is incompatible.
-----------------------------------------------------------------------------
* \endcode

 * @ingroup String
 */

// regex is a simple wrapper around Philip Hazel's pcre interface
// (see copyright notices in pcre.c)


#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <locale.h> 

#include "pcre.h"

#include "tb_global.h"

#include "Toolbox.h"
#include "Objects.h"
#include "String.h"
#include "Memory.h"



/** Tests a String against a regular expression pattern
 * Tries to match a regex on a given String_t.
 * @param string : subject
 * @param regex  : pattern (see pcre(7) for regex syntax)
 * @param options : 
 * - PCRE_CASELESS : case insensitive
 * - PCRE_MULTILINE : pattern run over lines breaks (^/$ applies on every lines)
 * - PCRE_DOTALL : match even LF
 * - PCRE_EXTENDED : not used
 * - PCRE_ANCHORED : '^' match only start of buffer
 * - PCRE_DOLLAR_ENDONLY : '$' match only end of buffer
 * - PCRE_UNGREEDY : toggle greediness to off (default is greedy)
 * - PCRE_NOBLANKS : Toolbox misc add : skip empty backrefs
 * - PCRE_MATCHMULTI : Toolbox misc add : apply pattern more than once if possible
 * - PCRE_MULTI : Toolbox misc add : idem
 * @return retcode_t (true if match)
 * @see tb_getRegex, tb_matchRegex, tb_Sed, tb_StrSplit
 * @ingroup Regex
 */
retcode_t tb_matchRegex(String_t string, char *regex, int options) {
  int            ovector[50];
  int            ovecsize             = sizeof(ovector)/sizeof(int);
  int            rc;
  const char   * errptr;
  int            erroffset            = 1;
	int            retval               = TB_ERR;
  pcre         * re                   = NULL;
	pcre_extra   * extra                = NULL;
	unsigned char *table                = NULL;

	if(tb_valid(string, TB_STRING, __FUNCTION__)) {

		if(re == NULL) {
			char *lc_ctype = setlocale(LC_CTYPE, NULL); 
			if( strcmp("C", lc_ctype) != 0) { 
				(char *)table = (char *)pcre_maketables(); 
			}

			if((re = pcre_compile(regex, options, &errptr, &erroffset, table)) == NULL) {
				tb_error("regex compile failed (%s)\n", errptr);
				return TB_KO;
			} 
		} 

		if( (rc=pcre_exec(re, extra, (char *)S2sz(string), 
											tb_getSize(string), 0, 0, ovector, ovecsize)) <-1 ) {
			tb_error("tb_matchRegex: regex exec error (%d)\n", rc);
			retval = TB_ERR;
		} else if( rc == 0 || rc == -1) {
			retval = TB_KO;
		}

		if(table) tb_xfree(table);
	}
  return retval;
}

int dbg_tb_getregex(char *func, char *file, int line, Vector_t  V, String_t  Str, char *regex, 
										int options) {
	set_tb_mdbg(func, file, line);
	return tb_getRegex(V, Str, regex, options);
}

/** Construct a Vector of matching regex's backreferences in a a String
 * Tries to match a regex on a given String_t. For each backreference subpattern found a new String_t is created and pushed on result Vector.
 *
 * Backreferences are patterns enclosed between parens as in "(a) ([bcd]+)". 
 *
 * See pcre(7) for full documentation on how to use perl-compatible regex.
 *
 * @param V : result Vector_t
 * @param Str : subject 
 * @param regex  : pattern (see pcre(7) for regex syntax)
 * @param options : 
 * - PCRE_CASELESS : case insensitive
 * - PCRE_MULTILINE : pattern run over lines breaks (^/$ applies on every lines)
 * - PCRE_DOTALL : match even LF
 * - PCRE_EXTENDED : not used
 * - PCRE_ANCHORED : '^' match only start of buffer
 * - PCRE_DOLLAR_ENDONLY : '$' match only end of buffer
 * - PCRE_UNGREEDY : toggle greediness to off (default is greedy)
 * - PCRE_NOBLANKS : Toolbox misc add : skip empty backrefs
 * - PCRE_MATCHMULTI : Toolbox misc add : apply pattern more than once if possible
 * - PCRE_MULTI : Toolbox misc add : idem
 * @return retcode_t (true if match)
 *
 * Examples: 
 * 
 * Warning: default pcre's behaviour is to send back at first a string matching whole expression,
 * then inner backreferences :
 * \code
 * String_t S = tb_String("a b c d e b c h i");
 * Vector_t V = tb_Vector();
 * tb_getRegex(V, S, "(c) (.)", 0);
 * will produce in V { "c d", "c", "d" }.
 * You must keep in mind that V[0] is _whole matching_ and not first backreference.
 * \endcode
 *
 * This behaviour is not retained when using PCRE_MATCHMULTI :
 * \code
 * String_t S = tb_String("a b c d e b c h i");
 * Vector_t V = tb_Vector();
 * tb_getRegex(V, S, "(c) (.)", PCRE_MATCHMULTI);
 * will produce in V { "c", "d", "c", "h" }.
 * \endcode
 *
 * @see tb_getRegex, tb_matchRegex, tb_Sed, tb_StrSplit
 * @ingroup Regex
*/
int tb_getRegex(Vector_t  V, String_t  Str, char *regex, int options) {

  int            ovector[50];
  int            ovecsize          = sizeof(ovector)/sizeof(int);
  int            rc                = 0, 
		i, 
		len, 
		string_len,
		multi             = 0,
		blanks            = 0;
	int            startoffset       = 0;
  const char   * errptr;
  int            erroffset         = 1;
  char         * buff;
	char         * string;
  pcre         * re                = NULL;
	pcre_extra   * extra                = NULL;
	unsigned char *table                = NULL;


	if(options & PCRE_MATCHMULTI) {
		multi = 1;
		options &= ~PCRE_MATCHMULTI;
	}
	if(options & TK_KEEP_BLANKS) {
		blanks = 1;
		options &= ~TK_KEEP_BLANKS;
	}


	if(tb_valid(V, TB_VECTOR, __FUNCTION__) &&
			tb_valid(Str, TB_STRING, __FUNCTION__) && regex  != NULL) {

		string = (char *)S2sz(Str);
		string_len = strlen(string);

		if(re == NULL) {
			char *lc_ctype = setlocale(LC_CTYPE, NULL); 
			if( strcmp("C", lc_ctype) != 0) { 
				(char *)table = (char *)pcre_maketables(); 
			}

			if((re = pcre_compile(regex, options, &errptr, &erroffset, table)) == NULL) {
				tb_error("regex compile failed (%s)\n", errptr);
				return TB_KO;
			}
		} 

		do {
			if(startoffset >= tb_getSize(Str)) break;
			if( (rc=pcre_exec(re, 
												extra, 
												string + startoffset, 
												string_len - startoffset, 
												0,
												0, 
												ovector, 
												ovecsize)) < -1 ) {

				tb_error("tb_getRegex: regex exec error (%d)\n", rc);
				break;
			} else if( rc == -1 ) {
				tb_debug("tb_getRegex: no matches\n");
				break;
			} else {
				int start = (multi) ? 1 : 0;
				for(i = start; i < rc; i++) {

					if( (len = pcre_get_substring(string +startoffset, ovector, rc, i, 
																				(const char **)&buff)) > 0) {
						tb_Push(V, tb_nString(len +1, "%s", buff));
						tb_xfree(buff);
					} else {
						if( blanks == 1 ) {
							tb_Push(V, tb_String(NULL));
						} else {
							tb_info("tb_getRegex(%s): len V[%d] = %d \n", regex, i, len);
						}
					}
				}
				startoffset += ovector[1];
			}
		} while(multi);

		if(table) tb_xfree(table);

		return tb_getSize(V);
	}
	return TB_ERR;
}


/**  Search and replace patterns into string
 *
 * Searches pattern may contains backreferences, which can be used
 * in replacement string with symbols $1,$2,...,$n each refering 
 * to substrings matched by search string (you may use all, part or none of this backreferences).
 *
 * Backreferences are patterns enclosed between parens as in "(a) ([bcd]+)". 
 *
 * See pcre(7) for full documentation on how to use perl-compatible regex.
 *
 * @param search : search regex pattern
 * @param replace : replacing pattern
 * @param text : subject 
 * @param options : 
 * - PCRE_CASELESS : case insensitive
 * - PCRE_MULTILINE : pattern run over lines breaks (^/$ applies on every lines)
 * - PCRE_DOTALL : match even LF
 * - PCRE_EXTENDED : not used
 * - PCRE_ANCHORED : '^' match only start of buffer
 * - PCRE_DOLLAR_ENDONLY : '$' match only end of buffer
 * - PCRE_UNGREEDY : toggle greediness to off (default is greedy)
 * - PCRE_NOBLANKS : Toolbox misc add : skip empty backrefs
 * - PCRE_MATCHMULTI : Toolbox misc add : apply pattern more than once if possible
 * - PCRE_MULTI : Toolbox misc add : idem
 * @return number of substitutions done or -1 on error
 *
 * Example: 
 * \code
 * search = '(\\S+)\\s(\\d+)'; replace = "$2 -> $1"
 * applied on string "abc 345 def" will produce "345 -> abc def" where $1 and $2
 * respectively match 'abc' and '345'
 * \endcode
 *
 * \code
 * invert two chars : matches every 'e' and immediate next char, then swap then :
 * S = tb_String("ma chaine de test a regexifier");
 * tb_Sed("(e)(.)", "[$2$1]", S, PCRE_MULTI);
 * produce : "ma chain[ e]d[ e]t[se]t a r[ge][xe]ifi[re]"
 * \endcode
 *
 * @see tb_getRegex, tb_matchRegex, tb_Sed, tb_StrSplit
 * @ingroup Regex
 */
int tb_Sed(char *search, char *replace, String_t  text, int options ) {
  int          ovector[50];
  int          ovecsize          = sizeof(ovector)/sizeof(int);
  int          rc                = 1, 
		           start,  
               end, 
               newstart          = 0;
  const char * errptr;
  int          erroffset         = 1;
  pcre       * re                = NULL;
  char       * raw;
  String_t     repl;
	int          cnt               = -1;
	int multi                      = options & PCRE_MULTI;
	int backref = 0;
	int lraw;
	unsigned char *table                = NULL;

	if(multi) {
		options &= ~PCRE_MULTI; // PCRE choke on foreign bits
	}

	if(tb_valid(text, TB_STRING, __FUNCTION__)) {
		string_members_t m = (string_members_t)((__members_t)
																						tb_getMembers(text, TB_STRING))->instance;

		raw  = m->data;
		lraw = m->size;


		if(re == NULL) {
			char *lc_ctype = setlocale(LC_CTYPE, NULL); 
			if( strcmp("C", lc_ctype) != 0) { 
				(char *)table = (char *)pcre_maketables(); 
			}

			if((re = pcre_compile(search, options, &errptr, &erroffset, table)) == NULL) {
				tb_error("regex compile failed (%s)\n", errptr);
				return TB_KO;
			}
		} 


		repl = tb_String("%s", replace);
		if( tb_matchRegex(repl, "\\$\\d", 0) ) backref = 1;
 
		while( rc > -1) {

		
			if( (rc = pcre_exec(re, NULL, raw, lraw, 0, 0, ovector, ovecsize)) <-1 ) {
				tb_error("tb_Sed: regex exec[%s] error (%d)\n", search, rc);
			} else if( rc == -1 ) {
				tb_debug("tb_Sed: no regex matches\n");
			} else {
				int lsearch;
				int lreplace;


				if(rc > 0) {
					char s[5];
					int n;
					char *buff;
					for(n = 1; n < rc; n++) {
						if( backref ) {
							if( pcre_get_substring(raw, ovector, rc, n, (const char **)&buff) > 0) {


								snprintf(s, 4, "$%d", n);
								tb_StrRepl(repl, s, buff);
								tb_xfree(buff);

							} else {
								snprintf(s, 4, "$%d", n);
								tb_StrRepl(repl, s, "");
							}
						}
					}
				
				}// else simple replace

				start      = ovector[0];
				end        = ovector[1];
				lsearch    = end - start;
				lreplace   = tb_getSize(repl);

				start += newstart;

				if( lsearch == lreplace ) {
					memcpy(m->data + start, S2sz(repl), lreplace);

				} else if( lsearch > lreplace ) {
					memcpy(m->data + start, S2sz(repl), lreplace);
					memmove(m->data + start + lreplace,
									m->data + start + lsearch,
									m->size - (start + lsearch) +1); //  +1 = '\0' terminal

					m->size -= lsearch - lreplace;

					/* Too big => reducing */
					if(m->allocated >= ( m->size * 3/*XSTR(S)->shrink_factor*/) ) {    
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
						m->allocated = ( m->size + 1 ) * m->grow_factor;
						s = tb_xrealloc(m->data, m->allocated);
						// if s == NULL :-> die horribly
						m->data = s;
        
					}

					memmove(m->data + start + lreplace,
									m->data + start + lsearch,
									oldsize - (start + lsearch) +1); //  +1 = '\0' terminal
					memcpy(m->data + start, S2sz(repl), lreplace);
				}

				newstart += lreplace + ovector[0];
			
				raw = m->data + newstart;
				lraw = tb_getSize(text) - newstart;
				cnt ++;
			}

			if(! multi) break;
			if( backref ) {
				tb_Free(repl);
				repl = tb_String("%s", replace);
			}
		}
		if(repl) tb_Free(repl);

		if(table) tb_xfree(table);

		return cnt;
	}
	return -1;
}


