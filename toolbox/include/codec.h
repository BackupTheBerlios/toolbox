// $Id: codec.h,v 1.1 2004/05/12 22:04:48 plg Exp $
//==============================================================
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

#ifndef CODEC_H
#define CODEC_H

#include "Toolbox.h"

/* forget about this !!
enum TB_CHARSETS {
	TB_CHARSET_LATIN1 = 0,
	TB_CHARSET_ASCII,
	TB_CHARSET_UTF8,
};

enum TB_ENCODINGS {
	TB_ENC_BARE = 0,
	TB_ENC_QP,
	TB_ENC_URL,
	TB_ENC_XML,
	TB_ENC_BASE64
};

*/
struct string_extra {
	int encoding;
	int charset;
};
typedef struct string_extra *string_extra_t;

#define XSTR(A) ((string_extra_t)A->extra)

int     base64_encode              (const void *data, int size, char **str);
int     base64_decode              (const char *str, void *data);
char  * decodeMIME                 (char *string, int length );


retcode_t latin1_to_utf8(char **utf8, unsigned char *latin1);
retcode_t encode_utf8_to_latin1(char **to, String_t from);
retcode_t encode_utf8_to_latin1(char **to, String_t from);
retcode_t utf8_to_latin1(char **latin1, unsigned char *utf8);

#endif
