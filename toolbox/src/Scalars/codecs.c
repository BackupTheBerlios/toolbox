//==========================================================
// $Id: codecs.c,v 1.1 2004/05/12 22:04:53 plg Exp $
//==========================================================
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

/*

This module is barely usable. Need a deep re-think an full rewrite. 

*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>

#include "Toolbox.h"
#include "Scalars.h" 
#include "codec.h"
#include "Objects.h"




/* base64 core functions from : */
/*
 * Copyright (c) 1995 - 1999 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

/**
 * @file codecs.c 
 */

/**
 * @defgroup Codecs Codecs
 * @ingroup String
 * String coder and decoder related methods and functions
 */


char * decodeMIME( char *string, int length );
char * decodeQP(char *in);

static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/** Encode char * string in Base64
 * 
 * @ingroup Codecs
 */
int tb_EncodeBase64(void *data, int size, char **str) {
	return base64_encode((const void *)data, size, str);
}

/** Decode Base64 string
 *
 * @ingroup Codecs
 */
int tb_DecodeBase64(char *str, void **data) {
	int rc;
	*data = tb_xcalloc(1, strlen(str));
	rc = base64_decode((const char *)str, *data);
	tb_xrealloc(*data, rc+1);
	return rc;
}

/** Transcode UTF8 encoded string to Latin1 charset
 *
 * @ingroup Codecs
 */
retcode_t tb_UTF8_to_Latin1          (String_t from) {
	if( TB_VALID(from, TB_STRING)) {
		char *s;
		if( utf8_to_latin1(&s, S2sz(from) ) == TB_OK ) {
			tb_StrAdd(tb_Clear(from), 0, "%s", s);
			tb_xfree(s);
			return TB_OK;
		}
	} 
	return TB_ERR;
}


/** Transcode Latin1 encoded string to UTF8 charset
 *
 * @ingroup Codecs
 */
retcode_t tb_Latin1_to_UTF8          (String_t from) {
	if( TB_VALID(from, TB_STRING)) {
		char *s;
		if( latin1_to_utf8(&s, S2sz(from) ) == TB_OK ) {
			tb_StrAdd(tb_Clear(from), 0, "%s", s);
			tb_xfree(s);
			return TB_OK;
		}
	}
	return TB_ERR;
}


/** Encode string as URL
 *
 * @ingroup Codecs
 */
String_t tb_UrlEncode(String_t S) {
	// space = '+'
	// non alphanum = %HH
	char *rez = tb_xmalloc(2*tb_getSize(S));
	char *s = S2sz(S);
	char *t = rez;
	while(*s) {
		if(*s == ' ') { 
			*t++ ='+'; 
		} else if(*s == '\n') { 
			sprintf(t, "%s", "%0D%0A");
			t+=6;
		}	else if(! isalnum(*s)) {
			sprintf(t, "%%%02X", (unsigned char)*s);
			t+=3;
		} else {
			*t++ = *s;
		}
		s++;
	}
	*t=0;
	tb_StrAdd(tb_Clear(S), 0, "%s", rez);
	tb_xfree(rez);

	return S;
}

/** Decode URL string
 *
 * @ingroup Codecs
 */
String_t tb_UrlDecode(String_t S) {
	char *rez = tb_xmalloc(tb_getSize(S));
	char *t = rez;
	char *s;
	tb_Sed("%0D%0A", "\n", S, PCRE_MULTI);
	s = S2sz(S);
	while(*s) {
		if(*s == '+') {
			*t++ = ' ';
		} else if(*s == '%') {
			int code;
			sscanf(s, "%%%02X", &code);
			*t++ = (unsigned char) code;
			s+=2;
		} else {
			*t++ = *s;
		}
		s++;
	}
	*t=0;

	tb_StrAdd(tb_Clear(S), 0, "%s", rez);
	tb_xfree(rez);

	return S;
}


//--------------------------------------

/** Encode latin1 char * to UTF8 encoding
 *
 */
retcode_t encode_latin1_to_utf8(char **to, String_t from) {
	if(! TB_VALID(from, TB_STRING)) return TB_ERR;
	return latin1_to_utf8(to, S2sz(from));
}


/** Encode UTF8 char * to latin1 encoding
 *
 * @ingroup Codecs
 */
retcode_t encode_utf8_to_latin1(char **to, String_t from) {
	if(! TB_VALID(from, TB_STRING)) return TB_ERR;
	return utf8_to_latin1(to, S2sz(from));
}



static int pos(char c)
{
  char *p;
  for(p = base64; *p; p++)
    if(*p == c) return p - base64;
  return -1;
}



int base64_encode(const void *data, int size, char **str)
{
  char *s, *p;
  int i;
  int c;
  const unsigned char *q;

  p = s = (char*)tb_xmalloc(size*4/3+4);
  if (p == NULL) return -1;

  q = (const unsigned char*)data;
  i=0;
  for(i = 0; i < size;){
    c=q[i++];
    c*=256;
    if(i < size)     c+=q[i];
    i++;
    c*=256;
    if(i < size)     c+=q[i];
    i++;
    p[0]=base64[(c&0x00fc0000) >> 18];
    p[1]=base64[(c&0x0003f000) >> 12];
    p[2]=base64[(c&0x00000fc0) >> 6];
    p[3]=base64[(c&0x0000003f) >> 0];
    if(i > size)     p[3]='=';
    if(i > size+1)   p[2]='=';
    p+=4;
  }
  *p=0;
  *str = s;
  return strlen(s);
}

int base64_decode(const char *str, void *data)
{
  const char *p;
  unsigned char *q;
  int c;
  int x;
  int done = 0;
  q=(unsigned char*)data;
  for(p=str; *p && !done; p+=4){
    x = pos(p[0]);
    if(x >= 0) {
			c = x;
		} else {
      done = 3;
      break;
    }
    c*=64;
    
    x = pos(p[1]);
    if(x >= 0)    c += x;
    else          return -1;

    c*=64;
    
    if(p[2] == '=') {
      done++;
		} else {
      x = pos(p[2]);
      if(x >= 0)  c += x;
      else      	return -1;
    }
    c*=64;
    
    if(p[3] == '=') {
			done++;
		} else { 
      if(done)   return -1;
      x = pos(p[3]);
      if(x >= 0) 	c += x;
      else       return -1;
    }
    if(done < 3)       *q++=(c&0x00ff0000)>>16;
      
    if(done < 2)       *q++=(c&0x0000ff00)>>8;
    if(done < 1)       *q++=(c&0x000000ff)>>0;
  }
  return q - (unsigned char*)data;
}



retcode_t latin1_to_utf8(char **utf8, unsigned char *latin1)
{
#warning fixme: range error
#if BUGGY_CODE
	int len = strlen(latin1)*2;
	int cur = 0;

	char *rez = tb_xcalloc(1, len);
	unsigned char c;
	


	while((c = *latin1++)) {
		if(cur +5 >= len) {
			len *=2;
			rez = tb_xrealloc(rez, len);
		}
		if (c < 0x80) {
			rez[cur++] = c;
		}	else if (c <  0x80) {
			rez[cur++] = (0xC0 | c>>6);
			rez[cur++] = (0x80 | (c & 0x3F));
		}	else if (c < 0x10000) {
			rez[cur++] = (0xE0 | (c>>12));
			rez[cur++] = (0x80 | ((c>>6) & 0x3F));
			rez[cur++] = (0x80 | (c & 0x3F));
		}	else if (c < 0x200000) {
			rez[cur++] = (0xF0 | c>>18);
			rez[cur++] = (0x80 | ((c>>12) & 0x3F));
			rez[cur++] = (0x80 | ((c>>6) & 0x3F));
			rez[cur++] = (0x80 | (c & 0x3F));
		}
	}
	len = strlen(rez);
	*utf8 = tb_xrealloc(rez, len+1);
	(*utf8)[len] = 0;
#endif
	return TB_OK;
}


retcode_t utf8_to_latin1(char **latin1, unsigned char *utf8) {
	unsigned char c;
	int len, i, iterations;
	unsigned char ch;

	char *u = utf8;
	char *latin_str, *s;

	if (utf8 == NULL) return TB_ERR;

	latin_str = tb_xcalloc(1, strlen(utf8)+1);
	s = latin_str;

	do {
		c = *u++;

		if ((c & 0xFE) == 0xFC) {
			c &= 0x01;
			iterations = 5;
		}	else if ((c & 0xFC) == 0xF8) {
			c &= 0x03;
			iterations = 4;
		}	else if ((c & 0xF8) == 0xF0) {
			c &= 0x07;
			iterations = 3;
		}	else if ((c & 0xF0) == 0xE0) {
			c &= 0x0F;
			iterations = 2;
		}	else if ((c & 0xE0) == 0xC0) {
			c &= 0x1F;
			iterations = 1;
		}	else if ((c & 0x80) == 0x80) {
			tb_xfree(s);
			tb_error("utf8_to_latin1: invalid value!");
			return TB_ERR;
		}	else {
			iterations = 0;
		}

		for(i = 0; i < iterations; i++) {
			ch = *u++;
			if ((ch & 0xC0) != 0x80) {
				tb_xfree(s);
				tb_error("utf8_to_latin1: invalid value!!");
				return TB_ERR;
			}
			c <<= 6;
			c |= ch & 0x3F;
		}

		*latin_str++ = c;
	} while( c != 0 );
	
	len = strlen(s);
	*latin1 = tb_xrealloc(s, len+1);
	(*latin1)[len] = 0;

	return TB_OK;
}   


/*
String_t tb_StrEncode(String_t S, int encoding) {
	// validate args

	char *s;
	if( encode[ encoding ] (&s, S) == TB_OK ) {
		tb_StrAdd(tb_Clear(S), 0, "%s", s);
		tb_xfree(s);
	}

	return S;
}
*/

int tb_getStrCharset( String_t S ) {
	tb_warn("tb_getStrCharset: *** deprecated ***\n");
	return 0;  
	/*
	if( TB_VALID(S, TB_STRING) ) {
			return XSTR(S)->charset;
	}
	return TB_ERR;
	*/
}

int tb_getStrEncoding( String_t S ) {
	tb_warn("tb_getStrCharset: *** deprecated ***\n");
	return 0;  
	/*
	if( TB_VALID(S, TB_STRING) ) {
			return XSTR(S)->encoding;
	}
	return TB_ERR;
	*/
}


// fixme: wrong ! see rfc !!
retcode_t tb_Latin1_to_Url           (String_t from) {
	if(TB_VALID(from, TB_STRING))	{
		int len = tb_getSize(from);
		int cur = 0;
		char *rez = tb_xcalloc(1, len * 2);
		char *s = S2sz(from);
		while( *s ) {
			if(cur == len) {
				len *=2;
				rez = tb_xrealloc(rez, len);
			}
			if(! isascii( *s )) {
				char buf[4];
				snprintf(buf, 4, "%02X",*s);
				sprintf(rez+cur, "%s", buf);
				cur +=3;
			} else {
				rez[cur++] = *s;
			}
		}
		return TB_OK;
	}
	return TB_ERR;
}

retcode_t tb_Url_to_Latin1           (String_t from) {return TB_ERR;}

/** Encode String_t to MIME Quoted-Printable
 * @ingroup Codecs
 */
retcode_t tb_Latin1_to_MimeQP        (String_t from) {
  if( TB_VALID(from, TB_STRING) ) {

    if(tb_getSize(from) > 0) {
      char tmp[3 * tb_getSize(from)];
      int i, j = 0;

      for(i = 0; i < tb_getSize(from); i++) {
				unsigned char c = *(S2sz(from) + i);
				if(c < 33 || c > 126 || c == '=') {	
					sprintf(tmp + j, "=%02X", (unsigned int)c);
					j += 3;
				} else {
					tmp[j++] = c;
				}
      }
      tmp[j] = '\0';
      
      tb_Clear(from);
      tb_StrAdd(from, 0, "%s", "=?iso-8859-1?Q?");
      tb_StrAdd(from, -1, "%s", tmp);
      tb_StrAdd(from, -1, "%s", "?=");
    }
    return TB_OK;
  } else {
    return TB_ERR;
  }
}

/** Decode MIME Quoted-Printable encoded string
 *
 * fixme: this is baroque and ugly (merge decodeMime)
 * @ingroup Codecs
 */
retcode_t tb_MimeQP_to_latin1        (String_t from) {
	char *t, *s = tb_xstrdup(S2sz(from));
	t = decodeMIME(s, tb_getSize(from));
	s = decodeQP(t);
	tb_xfree(t);
	if(s != NULL) {

		tb_StrAdd(tb_Clear(from), 0, "%s", s);
		tb_xfree(s);
		return TB_OK;
	}
	
	return TB_ERR;
}

/** Encode to MIME Base64
 *
 * @ingroup Codecs
 */
retcode_t tb_Latin1_to_MimeB64       (String_t from) {
	if( TB_VALID(from, TB_STRING) ) {
		char *s;

		if( base64_encode(S2sz(from), tb_getSize(from), &s) >0) {
			tb_StrAdd(tb_Clear(from), 0, "%s", s);
			tb_xfree(s);
			return TB_OK;
		}
	}
	return TB_ERR;
}

/** Decode MIME Base64
 *
 * @ingroup Codecs
 */
retcode_t tb_MimeB64_to_latin1       (String_t from) {
	if( TB_VALID(from, TB_STRING) ) {
		char *s;

		if( base64_encode(S2sz(from), tb_getSize(from), &s) >0) {
			tb_StrAdd(tb_Clear(from), 0, "%s", s);
			tb_xfree(s);
			return TB_OK;
		}
	}

	return TB_ERR;
}

/** Encode String in Base64
 *
 * @ingroup Codecs
 */
retcode_t tb_Str_to_Base64           (String_t from) {
	if( TB_VALID(from, TB_STRING) ) {
		char *s;

		if( base64_encode(S2sz(from), tb_getSize(from), &s) >0) {
			tb_StrAdd(tb_Clear(from), 0, "%s=", s); // fixme : this should be done in encoder
			tb_xfree(s);
			return TB_OK;
		}
	}
	return TB_ERR;
}

/** Decode Base64 String_t
// fixme: broken
* @ingroup Codecs
*/
retcode_t tb_Base64_to_Str           (String_t from) {
	if( TB_VALID(from, TB_STRING) ) {
		void *s;

		if( tb_DecodeBase64(S2sz(from), &s) >0) {
			tb_StrAdd(tb_Clear(from), 0, "%s", (char *)s);
			tb_xfree(s);
			return TB_OK;
		}
	}
	return TB_ERR;
}











//#if not_ready
char *decodeMIME( char *string, int length ) {
  char *iptr = string;
  char *oldptr;
  char *storage=tb_xmalloc(length+1);
  
  char *output = storage;
  
  char charset[129];
  char encoding[33];
  char blurb[129];
  char equal;
  int value;
  
  char didanything=FALSE;

  while (*iptr) {
    if (!strncmp(iptr, "=?", 2) &&
				(4 == sscanf(iptr+2, "%128[^?]?%32[^?]?%128[^?]?%c",
										 charset, encoding, blurb, &equal)) &&
				('=' == equal)) {
      /* This is a full, valid 'encoded-word'. Decode! */
      char *ptr=blurb;
			
      didanything=TRUE; /* yes, we decode something */
  
      /* we could've done this with a %n in the sscanf, but we know all
				 sscanfs don't grok that */

      iptr += 2+ strlen(charset) + 1 + strlen(encoding) + 1 + strlen(blurb) + 2;
			
      if (!strcasecmp("q", encoding)) {
				/* quoted printable decoding */
				
				for ( ; *ptr; ptr++ ) {
					switch ( *ptr ) {
					case '=':
						sscanf( ptr+1, "%02X", &value );
						*output++ = value;
						ptr += 2;
						break;
					case '_':
						*output++ = ' ';
						break;
					default:
						*output++ = *ptr;
						break;
					}
				}
      } else if (!strcasecmp("b", encoding)) {
				/* base64 decoding */
				output += base64_decode(ptr, output)-1;
      } else {
				/* unsupported encoding type */
				strcpy(output, "<unknown>");
				output += 9;
      }
			
      oldptr = iptr; /* save start position */
			
      while (*iptr && isspace(*iptr))
				iptr++; /* pass all whitespaces */
			
      /* if this is an encoded word here, we should skip the passed
				 whitespaces. If it isn't an encoded-word, we should include the
				 whitespaces in the output. */

      if (!strncmp(iptr, "=?", 2) &&
					(4 == sscanf(iptr+2, "%128[^?]?%32[^?]?%128[^?]?%c",
											 charset, encoding, blurb, &equal)) &&
					('=' == equal)) {
				continue; /* this IS an encoded-word, continue from here */
      }
      else
				/* this IS NOT an encoded-word, move back to the first whitespace */
				iptr = oldptr;
    } else {
      *output++ = *iptr++;   
		}
  }
  *output=0;
  
  if (didanything) {
    /* this check prevents unneccessary strsav() calls if not needed */
    tb_xfree(string); /* free old memory */
		
    return storage; /* return new */
  } else {
    strcpy(storage, string);
    tb_xfree (storage);
    return string;
  }
}
//#endif



#if 0

<!ENTITY lt     "&#38;#60;">  "<"
<!ENTITY gt     "&#62;">      ">"
<!ENTITY amp    "&#38;#38;">  "&"
<!ENTITY apos   "&#39;">      "'"
<!ENTITY quot   "&#34;">      "\""


String_t tb_XmlEncode(String_t S) {
	char *s, *t, *t2;
	int changed = 0;
	String_t T;
	if(! TB_VALID(S, TB_STRING)) return NULL;

	tb_Sed("&", "&amp;", S, NULL, PCRE_MULTI);
	tb_Sed("<", "&lt;", S, NULL, PCRE_MULTI);
	tb_Sed(">", "&gt;", S, NULL, PCRE_MULTI);
	tb_Sed("'", "&apos;", S, NULL, PCRE_MULTI);
	tb_Sed("\"", "&quot;", S, NULL, PCRE_MULTI);
	
	s = S2sz(S);
	T = tb_String(NULL);
	while( *s ) {
		if(! (isprint(*s) || isspace(*s) ) ) {
			changed = 1;
			break;
		}
	}
	if( changed ) {
		s = S2sz(S);
		while( *s ) {
			if(! (isprint(*s) || isspace(*s) ) ) {
				changed = 1;
				tb_StrAdd(T, -1, "&#%02d;", (unsigned char)*s);
			} else {
				tb_StrAdd(T, -1, "%c", *s);
			}
			s++;
		}
		tb_StrAdd(tb_Clear(S), 0, "%S", T);
	}
	tb_Free( T );

	return S;
}


#endif


/** Decode MIME Quoted-Printable
 *
 * @ingroup Codecs
 */
char * decodeQP(char *in) {
  char *s=in, *out, *t;
  unsigned char v;
  unsigned char QP[3] = {'\0', '\0', '\0'};

	setlocale(LC_CTYPE, "fr_FR");

  out = t = (char *)tb_xcalloc(sizeof(char), strsz(in));

  while(*s != '\0') {
    if(*s == '=' && (isxdigit(*(s+1))) && (isxdigit(*(s+2))) ) {
      QP[0] = (unsigned char)*(s+1);
      QP[1] = (unsigned char)*(s+2);
      sscanf(QP, "%02X", (unsigned int *)&v);
			//			if(isprint(v)) {
				if( v == 0x0D) {
					//                              trace("<%c%X%X>\n", *(s+3), *(s+4), *(s+5));
					if(*(s+3) == '=' && *(s+4) == 0x0A) {
						*out++ = '\n';
						s += isalnum( *(s+5)) ?  5 :  6;
						continue;
					}
				}
				*out++ = v;
				/*
			} else {
				tb_trace(TB_WARN, "decodeQP =%s -->%d (%c) \n", QP, v, v);
				*out++ = ' ';
			}
				*/
			s += 3;
    } else {
      if( *s == '=' && *(s+1)  == '\n') { s += 2; continue; };
      *out++ = *s++;
    }
  }

  return tb_xrealloc(t, strsz(t));
}




